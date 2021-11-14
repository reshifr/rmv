#include <vector>
#include <functional>
#include <cinttypes>
#include <string>
#include <iostream>
#include <iterator>

namespace rsfr {
  template <
    std::uint_fast8_t Exp,
    class T, bool Cached=false>
  class mv {
    public:
      using value_type = T;
      using reference = T&;
      using const_reference = const T&;
      using pointer = T*;
      using const_pointer = const T*;
      using size_type = std::uint_fast64_t;
      using difference_type = std::int_fast64_t;

    protected:
      using rdeep = std::uint_fast8_t;
      using rblock = std::uint_fast32_t;

      rdeep m_deep;
      rblock m_remainder;
      size_type m_peek;
      pointer* m_root;

      constexpr rblock block_size(void) {
        return static_cast<rblock>(1)<<Exp;
      }

      constexpr rblock block_end(void) {
        return block_size()-1;
      }

      constexpr rblock block_count(size_type size) {
        return (size>>Exp)+((size&block_end())==0 ? 0 : 1);
      }

      constexpr size_type peek(rdeep deep) {
        return (static_cast<size_type>(1)<<(Exp*(deep+1)))-1;
      }

      constexpr size_type mask(rdeep lvl) {
        return block_end()<<(Exp*lvl);
      }

      constexpr rblock jump(rdeep lvl, size_type index) {
        return (index&mask(lvl))>>(Exp*lvl);
      }

      mv(void) :
        m_deep(),
        m_remainder(),
        m_peek(),
        m_root() {}

      void build(pointer* root, rdeep lvl, rblock& num_block) {
        if( lvl==1 ) {
          for(auto i=jump(lvl, m_peek+block_size());
              num_block!=0 && i<block_size(); ++i) {
            root[i] = reinterpret_cast<pointer>(new value_type[block_size()]());
            m_peek += block_size();
            --num_block;
          }
          return;
        }
        for(auto i=jump(lvl, m_peek+block_size());
            num_block!=0 && i<block_size(); ++i) {
          if( root[i]==nullptr )
            root[i] = reinterpret_cast<pointer>(new pointer[block_size()]());
          build(reinterpret_cast<pointer*>(root[i]), lvl-1, num_block);
        }
      }

#ifdef RMV_DEBUG
    public:
      void debug(const_pointer* root, rdeep deep, std::ostream& stream) const {
        if( root==nullptr )
          return;
        std::string indent;
        for(size_type i=0; i<(deep<<1); ++i)
          indent += " ";
        stream<<indent<<"|";
        if( deep==m_deep ) {
          for(rblock i=0; i<block_size(); ++i)
            stream<<reinterpret_cast<const_pointer>(root)[i]<<"|";
          stream<<std::endl;
          return;
        }
        for(rblock i=0; i<block_size(); ++i)
          stream<<(static_cast<bool>(root[i]) ? "=" : " ")<<"|";
        stream<<std::endl;
        for(rblock i=0; i<block_size(); ++i)
          debug(reinterpret_cast<const_pointer*>(
            const_cast<pointer>(root[i])), deep+1, stream);
        return;
      }
#endif
  };

  template <
    std::uint_fast8_t Exp,
    class T, bool Cached=false>
  class rmv {};

  template <
    std::uint_fast8_t Exp,
    class T, bool Cached=false>
  class mvi {
    public:
      using value_type = T;
      using reference = T&;
      using const_reference = const T&;
      using pointer = T*;
      using const_pointer = const T*;
      using size_type = std::uint_fast64_t;
      using difference_type = std::int_fast64_t;
      using iterator_category = std::random_access_iterator_tag;

    private:
      size_type m_index;
      rmv<Exp, T, Cached> m_vector;

    public:
      // mvi(void) : m_index(), m_vector() {}
      mvi(rmv<Exp, T, Cached> vector) : m_index(), m_vector(vector) {}

      mvi(rmv<Exp, T, Cached> vector, size_type index) :
        m_index(index), m_vector(vector) {}

      // assignment =
      mvi& operator=(const mvi& it) {
        m_index = it.m_index;
        return *this;
      }

      // assignment +=
      mvi& operator+=(size_type index) {
        m_index += index;
        return *this;
      }

      // assignment -=
      mvi& operator-=(size_type index) {
        m_index -= index;
        return *this;
      }

      // increment
      mvi& operator++(void) {
        ++m_index;
        return *this;
      }
      mvi operator++(int) {
        ++m_index;
        return *this;
      }

      // decrement
      mvi& operator--(void) {
        --m_index;
        return *this;
      }
      mvi operator--(int) {
        --m_index;
        return *this;
      }

      // member access
      reference operator[](size_type index) {
        return m_vector[index];
      }

      // deference
      reference operator*(void) {
        return m_vector[m_index];
      }
      value_type operator*(void) const {
        return m_vector[m_index];
      }

      friend void swap(mvi& it_a, mvi& it_b) {
        mvi tmp = it_a;
        it_a = it_b;
        it_b = tmp;
      }

      // deference member access
      pointer operator->(void) {
        return &m_vector[m_index];
      }

      // comparison equal
      friend bool operator==(const mvi& it_a, const mvi& it_b) {
        return it_a.m_index==it_b.m_index;
      }

      // comparison not equal
      friend bool operator!=(const mvi& it_a, const mvi& it_b) {
        return it_a.m_index!=it_b.m_index;
      }

      // comparison lower than
      friend bool operator<(const mvi& it_a, const mvi& it_b) {
        return it_a.m_index<it_b.m_index;
      }

      // comparison upper than
      friend bool operator>(const mvi& it_a, const mvi& it_b) {
        return it_a.m_index>it_b.m_index;
      }

      // comparison lower than equal
      friend bool operator<=(const mvi& it_a, const mvi& it_b) {
        return it_a.m_index<=it_b.m_index;
      }

      // comparison upper than equal
      friend bool operator>=(const mvi& it_a, const mvi& it_b) {
        return it_a.m_index>=it_b.m_index;
      }

      // arithmetic add
      friend mvi operator+(const mvi& it, size_type index) {
        return mvi(it.m_vector, it.m_index+index);
      }
      friend mvi operator+(size_type index, const mvi& it) {
        return mvi(it.m_vector, index+it.m_index);
      }

      // arithmetic sub
      friend mvi operator-(const mvi& it, size_type index) {
        return mvi(it.m_vector, it.m_index-index);
      }
      friend difference_type operator-(mvi it_a, mvi it_b) {
        return it_a.m_index-it_b.m_index;
      }
  };

  template <std::uint_fast8_t Exp, class T>
  class rmv<Exp, T> : public mv<Exp, T> {
    using typename mv<Exp, T>::value_type;
    using typename mv<Exp, T>::reference;
    using typename mv<Exp, T>::const_reference;
    using typename mv<Exp, T>::pointer;
    using typename mv<Exp, T>::const_pointer;
    using typename mv<Exp, T>::size_type;
    using typename mv<Exp, T>::difference_type;

    using typename mv<Exp, T>::rdeep;
    using typename mv<Exp, T>::rblock;

    using mv<Exp, T>::m_deep;
    using mv<Exp, T>::m_remainder;
    using mv<Exp, T>::m_peek;
    using mv<Exp, T>::m_root;

    using mv<Exp, T>::block_size;
    using mv<Exp, T>::block_end;
    using mv<Exp, T>::block_count;
    using mv<Exp, T>::peek;
    using mv<Exp, T>::mask;
    using mv<Exp, T>::jump;
    using mv<Exp, T>::build;

    public:
      rmv(void) : mv<Exp, T>() {}

      explicit rmv(size_type num) : rmv() {
        extend(num);
      }

      rmv(const std::initializer_list<value_type>& init) : rmv() {
        extend(init.size());
      }

      void extend(size_type num) {
        auto num_block = block_count(num);
        if( num_block==0 )
          return;
        if( m_peek==0 ) {
          m_root = reinterpret_cast<pointer*>(
            new value_type[block_size()]());
          m_peek += block_end();
          --num_block;
        }
        while( num_block!=0 ) {
          if( m_peek==peek(m_deep) ) {
            auto block = m_root;
            m_root = new pointer[block_size()]();
            *m_root = reinterpret_cast<pointer>(block);
            ++m_deep;
          }
          build(m_root, m_deep, num_block);
        }
      }

      reference operator[](size_type index) {
        auto block = m_root;
        for(rdeep lvl=m_deep; lvl>0; --lvl)
          block = reinterpret_cast<pointer*>(block[jump(lvl, index)]);
        return reinterpret_cast<pointer>(block)[jump(0, index)];
      }

      mvi<Exp, T> begin(void) {
        return mvi<Exp, T>(*this);
      }

      mvi<Exp, T> end(void) {
        return mvi<Exp, T>(*this, m_peek);
      }

#ifdef RMV_DEBUG
      friend std::ostream& operator<<(
        std::ostream& stream, const rmv& vector) {
        stream<<std::endl;
        stream<<"deep  = "<<static_cast<size_type>(
          vector.m_deep)<<std::endl;
        stream<<"peek  = "<<static_cast<size_type>(
          vector.m_peek)<<std::endl;
        stream<<"max   = "<<static_cast<size_type>(
          vector.peek(vector.m_deep)+1)<<std::endl;
        stream<<"root  = "<<static_cast<void*>(vector.m_root)<<std::endl;
        stream<<std::endl;
        vector.debug(const_cast<const_pointer*>(vector.m_root), 0, stream);
        return stream;
      }
#endif
  };

  template <std::uint_fast8_t Exp, class T>
  class rmv<Exp, T, true> : public mv<Exp, T> {
    using typename mv<Exp, T>::value_type;
    using typename mv<Exp, T>::reference;
    using typename mv<Exp, T>::const_reference;
    using typename mv<Exp, T>::pointer;
    using typename mv<Exp, T>::const_pointer;
    using typename mv<Exp, T>::size_type;
    using typename mv<Exp, T>::difference_type;

    using typename mv<Exp, T>::rdeep;
    using typename mv<Exp, T>::rblock;

    using mv<Exp, T>::m_deep;
    using mv<Exp, T>::m_remainder;
    using mv<Exp, T>::m_peek;
    using mv<Exp, T>::m_root;

    using mv<Exp, T>::block_size;
    using mv<Exp, T>::block_end;
    using mv<Exp, T>::block_count;
    using mv<Exp, T>::peek;
    using mv<Exp, T>::mask;
    using mv<Exp, T>::jump;
    using mv<Exp, T>::build;

    private:
      size_type m_tag;
      value_type* m_cache;
      
      constexpr size_type tag(size_type index) {
        return index>>Exp;
      }

    public:
      rmv(void) : mv<Exp, T>(), m_cache() {}

      explicit rmv(size_type num) : rmv() {
        extend(num);
      }

      rmv(const std::initializer_list<value_type>& init) : rmv() {
        extend(init.size());
      }

      void extend(size_type num) {
        auto num_block = block_count(num);
        if( num_block==0 )
          return;
        if( m_peek==0 ) {
          m_cache = new value_type[block_size()]();
          m_root = reinterpret_cast<value_type**>(m_cache);
          m_peek += block_end();
          --num_block;
        }
        while( num_block!=0 ) {
          if( m_peek==peek(m_deep) ) {
            auto block = m_root;
            m_root = new value_type*[block_size()]();
            *m_root = reinterpret_cast<value_type*>(block);
            ++m_deep;
          }
          build(m_root, m_deep, num_block);
        }
      }

      reference operator[](size_type index) {
        if( m_tag==tag(index) )
          return m_cache[jump(0, index)];
        auto block = m_root;
        for(rdeep lvl=m_deep; lvl>0; --lvl)
          block = reinterpret_cast<value_type**>(block[jump(lvl, index)]);
        m_tag = tag(index);
        m_cache = reinterpret_cast<value_type*>(block);
        return reinterpret_cast<value_type*>(block)[jump(0, index)];
      }

#ifdef RMV_DEBUG
      friend std::ostream& operator<<(
        std::ostream& stream, const rmv& vector) {
        stream<<std::endl;
        stream<<"deep  = "<<static_cast<size_type>(
          vector.m_deep)<<std::endl;
        stream<<"peek  = "<<static_cast<size_type>(
          vector.m_peek)<<std::endl;
        stream<<"tag   = "<<static_cast<size_type>(
          vector.m_tag)<<std::endl;
        stream<<"max   = "<<static_cast<size_type>(
          vector.peek(vector.m_deep)+1)<<std::endl;
        stream<<"root  = "<<static_cast<void*>(vector.m_root)<<std::endl;
        stream<<"cache = "<<static_cast<void*>(vector.m_cache)<<std::endl;
        stream<<std::endl;
        vector.debug(const_cast<const_pointer*>(vector.m_root), 0, stream);
        return stream;
      }
#endif
  };
}
