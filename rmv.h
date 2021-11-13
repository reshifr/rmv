#include <vector>
#include <functional>
#include <cinttypes>
#include <string>
#include <iostream>

namespace rsfr {
  template <
    std::uint_fast8_t Exp,
    class T, bool Cached=false>
  class mv {
    protected:
      std::uint_fast8_t m_deep;
      std::uint_fast32_t m_remainder;
      std::size_t m_peek;
      T** m_root;

      constexpr std::uint_fast32_t block_size(void) {
        return static_cast<std::uint_fast32_t>(1)<<Exp;
      }

      constexpr std::uint_fast32_t block_end(void) {
        return block_size()-1;
      }

      constexpr std::uint_fast32_t block_count(std::size_t size) {
        return (size>>Exp)+((size&block_end())==0 ? 0 : 1);
      }

      constexpr std::size_t size(std::uint_fast8_t deep) {
        return static_cast<std::size_t>(1)<<(Exp*(deep+1));
      }

      constexpr std::size_t end(std::uint_fast8_t deep) {
        return (static_cast<std::size_t>(1)<<(Exp*(deep+1)))-1;
      }

      constexpr std::size_t mask(std::uint_fast8_t lvl) {
        return block_end()<<(Exp*lvl);
      }

      constexpr std::uint_fast32_t jump(
        std::uint_fast8_t lvl, std::size_t index) {
        return (index&mask(lvl))>>(Exp*lvl);
      }

      mv(void) :
        m_deep(),
        m_remainder(),
        m_peek(),
        m_root() {}

      void build(
        T** root,
        std::uint_fast8_t lvl,
        std::uint_fast32_t& num_block) {
        if( lvl==1 ) {
          for(std::uint_fast32_t i=jump(lvl, m_peek+block_size());
              num_block!=0 && i<block_size(); ++i) {
            root[i] = reinterpret_cast<T*>(new T[block_size()]());
            m_peek += block_size();
            --num_block;
          }
          return;
        }
        for(std::uint_fast32_t i=jump(lvl, m_peek+block_size());
            num_block!=0 && i<block_size(); ++i) {
          if( root[i]==nullptr )
            root[i] = reinterpret_cast<T*>(new T*[block_size()]());
          build(reinterpret_cast<T**>(root[i]), lvl-1, num_block);
        }
      }

#ifdef RMV_DEBUG
    public:
      void debug(T** root,
        std::uint_fast8_t deep, std::ostream& stream) const {
        if( root==nullptr )
          return;
        std::string indent;
        for(std::size_t i=0; i<(deep<<1); ++i)
          indent += " ";
        stream<<indent<<"|";
        if( deep==m_deep ) {
          for(std::uint_fast32_t i=0; i<block_size(); ++i)
            stream<<reinterpret_cast<T*>(root)[i]<<"|";
          stream<<std::endl;
          return;
        }
        for(std::uint_fast32_t i=0; i<block_size(); ++i)
          stream<<(static_cast<bool>(root[i]) ? "=" : " ")<<"|";
        stream<<std::endl;
        for(std::uint_fast32_t i=0; i<block_size(); ++i)
          debug(reinterpret_cast<T**>(root[i]), deep+1, stream);
        return;
      }
#endif
  };

  template <
    std::uint_fast8_t Exp,
    class T, bool Cached=false>
  class rmv {};

  template <std::uint_fast8_t Exp, class T>
  class rmv<Exp, T> : public mv<Exp, T> {
    public:
      // T& operator[](std::size_t index) const {
      //   T** block = mv<Exp, T>::m_root;
      //   for(std::uint_fast8_t lvl=mv<Exp, T>::m_deep; lvl>0; --lvl)
      //     block = reinterpret_cast<T**>(block[mv<Exp, T>::jump(lvl, index)]);
      //   return reinterpret_cast<T*>(block)[mv<Exp, T>::jump(0, index)];
      // }
  };

  template <std::uint_fast8_t Exp, class T>
  class rmv<Exp, T, true> : public mv<Exp, T> {
    private:
      std::size_t m_tag;
      T* m_cache;
      
      constexpr std::size_t tag(std::size_t index) {
        return index>>Exp;
      }

    public:
      rmv(void) : mv<Exp, T>(), m_cache() {}

      explicit rmv(std::size_t num) : rmv() {
        extend(num);
      }

      rmv(const std::initializer_list<T>& init) : rmv() {
        extend(init.size());
      }

      void extend(std::size_t num) {
        auto num_block = mv<Exp, T>::block_count(num);
        if( num_block==0 )
          return;
        if( mv<Exp, T>::m_peek==0 ) {
          m_cache = new T[mv<Exp, T>::block_size()]();
          mv<Exp, T>::m_root = reinterpret_cast<T**>(m_cache);
          mv<Exp, T>::m_peek += mv<Exp, T>::block_end();
          --num_block;
        }
        while( num_block!=0 ) {
          if( mv<Exp, T>::m_peek==mv<Exp, T>::end(mv<Exp, T>::m_deep) ) {
            T** block = mv<Exp, T>::m_root;
            mv<Exp, T>::m_root = new T*[mv<Exp, T>::block_size()]();
            *mv<Exp, T>::m_root = reinterpret_cast<T*>(block);
            ++mv<Exp, T>::m_deep;
          }
          mv<Exp, T>::build(mv<Exp, T>::m_root,mv<Exp, T>:: m_deep, num_block);
        }
      }

      T& operator[](std::size_t index) {
        if( m_tag==tag(index) )
          return m_cache[mv<Exp, T>::jump(0, index)];
        T** block = mv<Exp, T>::m_root;
        for(std::uint_fast8_t lvl=mv<Exp, T>::m_deep; lvl>0; --lvl)
          block = reinterpret_cast<T**>(block[mv<Exp, T>::jump(lvl, index)]);
        m_tag = tag(index);
        m_cache = reinterpret_cast<T*>(block);
        return reinterpret_cast<T*>(block)[mv<Exp, T>::jump(0, index)];
      }

#ifdef RMV_DEBUG
      friend std::ostream& operator<<(
        std::ostream& stream, const rmv& vector) {
        stream<<std::endl;
        stream<<"deep  = "<<static_cast<std::size_t>(
          vector.m_deep)<<std::endl;
        stream<<"peek  = "<<static_cast<std::size_t>(
          vector.m_peek)<<std::endl;
        stream<<"tag   = "<<static_cast<std::size_t>(
          vector.m_tag)<<std::endl;
        stream<<"max   = "<<static_cast<std::size_t>(
          vector.end(vector.m_deep)+1)<<std::endl;
        stream<<"root  = "<<static_cast<void*>(vector.m_root)<<std::endl;
        stream<<"cache = "<<static_cast<void*>(vector.m_cache)<<std::endl;
        stream<<std::endl;
        vector.debug(vector.m_root, 0, stream);
        return stream;
      }
#endif
  };
}
