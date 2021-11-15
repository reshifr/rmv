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
    protected:
      using value_type = T;
      using reference = T&;
      using const_reference = const T&;
      using pointer = T*;
      using const_pointer = const T*;
      using size_type = std::uint_fast64_t;
      using difference_type = std::int_fast64_t;

      using rdeep = std::uint_fast8_t;
      using rblock = std::uint_fast32_t;

      rdeep m_deep;
      rblock m_free;
      size_type m_peek;
      pointer* m_root;

      constexpr rblock block_size(void) {
        return static_cast<rblock>(1)<<Exp; }
      constexpr rblock block_end(void) {
        return block_size()-1; }
      constexpr rblock block_count(size_type num) {
        return (num>>Exp)+((num&block_end())==0 ? 0 : 1); }
      constexpr size_type peek(rdeep deep) {
        return (static_cast<size_type>(1)<<(Exp*(deep+1)))-1; }
      constexpr size_type mask(rdeep lvl) {
        return block_end()<<(Exp*lvl); }
      constexpr rblock jump(rdeep lvl, size_type index) {
        return (index&mask(lvl))>>(Exp*lvl); }

      mv(void) :
        m_deep(),
        m_free(),
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
    protected:
      using size_type = std::uint_fast64_t;
      using difference_type = std::int_fast64_t;
      using iterator_category = std::random_access_iterator_tag;

      difference_type m_pos;
      rmv<Exp, T, Cached>* m_vector;

    public:
      mvi(void) :
        m_pos(), m_vector() {}
      explicit mvi(rmv<Exp, T, Cached>* vector) :
        m_pos(), m_vector(vector) {}
      explicit mvi(rmv<Exp, T, Cached>* vector, difference_type off) :
        m_pos(off), m_vector(vector) {}

      bool operator==(const mvi& it) const {
        return m_pos==it.m_pos; }
      bool operator!=(const mvi& it) const {
        return m_pos!=it.m_pos; }
      bool operator<(const mvi& it) const {
        return m_pos<it.m_pos; }
      bool operator<=(const mvi& it) const {
        return m_pos<=it.m_pos; }
      bool operator>(const mvi& it) const {
        return m_pos>it.m_pos; }
      bool operator>=(const mvi& it) const {
        return m_pos>=it.m_pos; }
  };

  template <
    std::uint_fast8_t Exp,
    class T, bool Cached=false>
  class rmvi : public mvi<Exp, T, Cached> {
    using mvi<Exp, T, Cached>::m_pos;
    using mvi<Exp, T, Cached>::m_vector;

    public:
      using value_type = T;
      using reference = T&;
      using pointer = T*;
      using size_type = typename mvi<Exp, T, Cached>::size_type;
      using difference_type = typename mvi<Exp, T, Cached>::difference_type;
      using iterator_category = typename mvi<Exp, T, Cached>::iterator_category;

      rmvi(void) : mvi<Exp, T, Cached>() {}
      explicit rmvi(rmv<Exp, T, Cached>* vector) :
        mvi<Exp, T, Cached>(vector, 0) {}
      explicit rmvi(rmv<Exp, T, Cached>* vector, difference_type off) :
        mvi<Exp, T, Cached>(vector, off) {}

      reference operator*(void) const {
        return (*m_vector)[m_pos]; }
      pointer operator->(void) const {
        return &(*m_vector)[m_pos]; }
      reference operator[](difference_type off) const {
        return (*m_vector)[off]; }

      rmvi& operator++(void) {
        ++m_pos; return *this; }
      rmvi operator++(int) {
        return rmvi(m_vector, m_pos++); }
      rmvi& operator--(void) {
        --m_pos; return *this; }
      rmvi operator--(int) {
        return rmvi(m_vector, m_pos--); }

      rmvi& operator+=(difference_type off) {
        m_pos += off; return *this; }
      rmvi operator+(difference_type off) const {
        return rmvi(m_vector, m_pos+off); }
      friend rmvi operator+(difference_type off, const rmvi& it) {
        return rmvi(it.m_vector, off+it.m_pos); }
      rmvi& operator-=(difference_type off) {
        m_pos -= off; return *this; }
      rmvi operator-(difference_type off) const {
        return rmvi(m_vector, m_pos-off); }
      difference_type operator-(const rmvi& it) const {
        return m_pos-it.m_pos; }
  };

  template <
    std::uint_fast8_t Exp,
    class T, bool Cached=false>
  class rmvci : public mvi<Exp, T, Cached> {
    using mvi<Exp, T, Cached>::m_pos;
    using mvi<Exp, T, Cached>::m_vector;

    public:
      using value_type = T;
      using const_reference = const T&;
      using const_pointer = const T*;
      using size_type = typename mvi<Exp, T, Cached>::size_type;
      using difference_type = typename mvi<Exp, T, Cached>::difference_type;
      using iterator_category = typename mvi<Exp, T, Cached>::iterator_category;

      rmvci(void) : mvi<Exp, T, Cached>() {}
      explicit rmvci(rmv<Exp, T, Cached>* vector) :
        mvi<Exp, T, Cached>(vector, 0) {}
      explicit rmvci(rmv<Exp, T, Cached>* vector, difference_type off) :
        mvi<Exp, T, Cached>(vector, off) {}

      const_reference operator*(void) const {
        return (*m_vector)[m_pos]; }
      const_pointer operator->(void) const {
        return &(*m_vector)[m_pos]; }
      const_reference operator[](difference_type off) const {
        return (*m_vector)[off]; }

      rmvci& operator++(void) {
        ++m_pos; return *this; }
      rmvci operator++(int) {
        return rmvci(m_vector, m_pos++); }
      rmvci& operator--(void) {
        --m_pos; return *this; }
      rmvci operator--(int) {
        return rmvci(m_vector, m_pos--); }

      rmvci& operator+=(difference_type off) {
        m_pos += off; return *this; }
      rmvci operator+(difference_type off) const {
        return rmvci(m_vector, m_pos+off); }
      friend rmvci operator+(difference_type off, const rmvci& it) {
        return rmvci(it.m_vector, off+it.m_pos); }
      rmvci& operator-=(difference_type off) {
        m_pos -= off; return *this; }
      rmvci operator-(difference_type off) const {
        return rmvci(m_vector, m_pos-off); }
      difference_type operator-(const rmvci& it) const {
        return m_pos-it.m_pos; }
  };

  template <
    std::uint_fast8_t Exp,
    class T, bool Cached=false>
  class rmvri : public mvi<Exp, T, Cached> {
    using mvi<Exp, T, Cached>::m_pos;
    using mvi<Exp, T, Cached>::m_vector;

    public:
      using value_type = T;
      using reference = T&;
      using pointer = T*;
      using size_type = typename mvi<Exp, T, Cached>::size_type;
      using difference_type = typename mvi<Exp, T, Cached>::difference_type;
      using iterator_category = typename mvi<Exp, T, Cached>::iterator_category;

      rmvri(void) : mvi<Exp, T, Cached>() {}
      explicit rmvri(rmv<Exp, T, Cached>* vector) :
        mvi<Exp, T, Cached>(vector, vector->size()-1) {}
      explicit rmvri(rmv<Exp, T, Cached>* vector, difference_type off) :
        mvi<Exp, T, Cached>(vector, off) {}

      reference operator*(void) const {
        return (*m_vector)[m_pos]; }
      pointer operator->(void) const {
        return &(*m_vector)[m_pos]; }
      reference operator[](difference_type off) const {
        return (*m_vector)[off]; }

      rmvri& operator++(void) {
        --m_pos; return *this; }
      rmvri operator++(int) {
        return rmvri(m_vector, m_pos--); }
      rmvri& operator--(void) {
        ++m_pos; return *this; }
      rmvri operator--(int) {
        return rmvri(m_vector, m_pos++); }

      rmvri& operator+=(difference_type off) {
        m_pos -= off; return *this; }
      rmvri operator+(difference_type off) const {
        return rmvri(m_vector, m_pos-off); }
      friend rmvri operator+(difference_type off, const rmvri& it) {
        return rmvri(it.m_vector, off-it.m_pos); }
      rmvri& operator-=(difference_type off) {
        m_pos += off; return *this; }
      rmvri operator-(difference_type off) const {
        return rmvri(m_vector, m_pos+off); }
      difference_type operator-(const rmvri& it) const {
        return m_pos+it.m_pos; }
  };

  template <
    std::uint_fast8_t Exp,
    class T, bool Cached=false>
  class rmvcri : public mvi<Exp, T, Cached> {
    using mvi<Exp, T, Cached>::m_pos;
    using mvi<Exp, T, Cached>::m_vector;

    public:
      using value_type = T;
      using const_reference = const T&;
      using const_pointer = const T*;
      using size_type = typename mvi<Exp, T, Cached>::size_type;
      using difference_type = typename mvi<Exp, T, Cached>::difference_type;
      using iterator_category = typename mvi<Exp, T, Cached>::iterator_category;

      rmvcri(void) : mvi<Exp, T, Cached>() {}
      explicit rmvcri(rmv<Exp, T, Cached>* vector) :
        mvi<Exp, T, Cached>(vector, vector->size()-1) {}
      explicit rmvcri(rmv<Exp, T, Cached>* vector, difference_type off) :
        mvi<Exp, T, Cached>(vector, off) {}

      const_reference operator*(void) const {
        return (*m_vector)[m_pos]; }
      const_pointer operator->(void) const {
        return &(*m_vector)[m_pos]; }
      const_reference operator[](difference_type off) const {
        return (*m_vector)[off]; }

      rmvcri& operator++(void) {
        --m_pos; return *this; }
      rmvcri operator++(int) {
        return rmvcri(m_vector, m_pos--); }
      rmvcri& operator--(void) {
        ++m_pos; return *this; }
      rmvcri operator--(int) {
        return rmvcri(m_vector, m_pos++); }

      rmvcri& operator+=(difference_type off) {
        m_pos -= off; return *this; }
      rmvcri operator+(difference_type off) const {
        return rmvcri(m_vector, m_pos-off); }
      friend rmvcri operator+(difference_type off, const rmvcri& it) {
        return rmvcri(it.m_vector, off-it.m_pos); }
      rmvcri& operator-=(difference_type off) {
        m_pos += off; return *this; }
      rmvcri operator-(difference_type off) const {
        return rmvcri(m_vector, m_pos+off); }
      difference_type operator-(const rmvcri& it) const {
        return m_pos+it.m_pos; }
  };

  template <std::uint_fast8_t Exp, class T>
  class rmv<Exp, T> : public mv<Exp, T> {
    using typename mv<Exp, T>::rdeep;
    using typename mv<Exp, T>::rblock;

    using mv<Exp, T>::m_deep;
    using mv<Exp, T>::m_free;
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
      using value_type = typename mv<Exp, T>::value_type;
      using reference = typename mv<Exp, T>::reference;
      using const_reference = typename mv<Exp, T>::const_reference;
      using pointer = typename mv<Exp, T>::pointer;
      using const_pointer = typename mv<Exp, T>::const_pointer;
      using size_type = typename mv<Exp, T>::size_type;
      using difference_type = typename mv<Exp, T>::difference_type;
      using iterator = rmvi<Exp, T>;
      using const_iterator = rmvci<Exp, T>;

    public:
      void extend(size_type num) {
        auto new_size = size()+num;
        if( num<=m_free ) {
          m_free -= num;
          return;
        } else
          num -= m_free;
        auto num_block = block_count(num);
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
        m_free = capacity()-new_size;
      }
    
    public:
      rmv(void) : mv<Exp, T>() {}

      explicit rmv(size_type num) : rmv() {
        extend(num);
      }

      rmv(const std::initializer_list<value_type>& init) : rmv() {
        extend(init.size());
      }

      reference operator[](size_type index) {
        auto block = m_root;
        for(rdeep lvl=m_deep; lvl>0; --lvl)
          block = reinterpret_cast<pointer*>(block[jump(lvl, index)]);
        return reinterpret_cast<pointer>(block)[jump(0, index)];
      }

      constexpr size_type size(void) {
        return capacity()-m_free; }
      constexpr size_type capacity(void) {
        return m_root==nullptr ? 0 : m_peek+1; }

      rmvi<Exp, T> begin(void) {
        return rmvi<Exp, T>(this); }
      rmvri<Exp, T> rbegin(void) {
        return rmvri<Exp, T>(this); }
      rmvci<Exp, T> cbegin(void) {
        return rmvci<Exp, T>(this); }
      rmvcri<Exp, T> crbegin(void) {
        return rmvcri<Exp, T>(this); }

      rmvi<Exp, T> end(void) {
        return rmvi<Exp, T>(this, size()); }
      rmvri<Exp, T> rend(void) {
        return rmvri<Exp, T>(this, -1); }
      rmvci<Exp, T> cend(void) {
        return rmvci<Exp, T>(this, size()); }
      rmvcri<Exp, T> crend(void) {
        return rmvcri<Exp, T>(this, -1); }

#ifdef RMV_DEBUG
      friend std::ostream& operator<<(
        std::ostream& stream, const rmv& vector) {
        stream<<std::endl;
        stream<<"deep     = "<<static_cast<size_type>(
          vector.m_deep)<<std::endl;
        stream<<"free     = "<<static_cast<size_type>(
          vector.m_free)<<std::endl;
        stream<<"peek     = "<<static_cast<size_type>(
          vector.m_peek)<<std::endl;
        stream<<"size     = "<<static_cast<size_type>(
          vector.size())<<std::endl;
        stream<<"capacity = "<<static_cast<size_type>(
          vector.capacity())<<std::endl;
        stream<<"max      = "<<static_cast<size_type>(
          vector.peek(vector.m_deep)+1)<<std::endl;
        stream<<"root     = "<<static_cast<void*>(vector.m_root)<<std::endl;
        stream<<std::endl;
        vector.debug(const_cast<const_pointer*>(vector.m_root), 0, stream);
        return stream;
      }
#endif
  };

//   template <std::uint_fast8_t Exp, class T>
//   class rmv<Exp, T, true> : public mv<Exp, T> {
//     using typename mv<Exp, T>::value_type;
//     using typename mv<Exp, T>::reference;
//     using typename mv<Exp, T>::const_reference;
//     using typename mv<Exp, T>::pointer;
//     using typename mv<Exp, T>::const_pointer;
//     using typename mv<Exp, T>::size_type;
//     using typename mv<Exp, T>::difference_type;

//     using typename mv<Exp, T>::rdeep;
//     using typename mv<Exp, T>::rblock;

//     using mv<Exp, T>::m_deep;
//     using mv<Exp, T>::m_remainder;
//     using mv<Exp, T>::m_peek;
//     using mv<Exp, T>::m_root;

//     using mv<Exp, T>::block_size;
//     using mv<Exp, T>::block_end;
//     using mv<Exp, T>::block_count;
//     using mv<Exp, T>::peek;
//     using mv<Exp, T>::mask;
//     using mv<Exp, T>::jump;
//     using mv<Exp, T>::build;

//     private:
//       using value_type = typename mv<Exp, T>::value_type;
//       using reference = typename mv<Exp, T>::reference;
//       using const_reference = typename mv<Exp, T>::const_reference;
//       using pointer = typename mv<Exp, T>::pointer;
//       using const_pointer = typename mv<Exp, T>::const_pointer;
//       using size_type = typename mv<Exp, T>::size_type;
//       using difference_type = typename mv<Exp, T>::difference_type;

//       size_type m_tag;
//       value_type* m_cache;
      
//       constexpr size_type tag(size_type index) {
//         return index>>Exp;
//       }

//     public:
//       rmv(void) : mv<Exp, T>(), m_cache() {}

//       explicit rmv(size_type num) : rmv() {
//         extend(num);
//       }

//       rmv(const std::initializer_list<value_type>& init) : rmv() {
//         extend(init.size());
//       }

//       void extend(size_type num) {
//         auto num_block = block_count(num);
//         if( num_block==0 )
//           return;
//         if( m_peek==0 ) {
//           m_cache = new value_type[block_size()]();
//           m_root = reinterpret_cast<value_type**>(m_cache);
//           m_peek += block_end();
//           --num_block;
//         }
//         while( num_block!=0 ) {
//           if( m_peek==peek(m_deep) ) {
//             auto block = m_root;
//             m_root = new value_type*[block_size()]();
//             *m_root = reinterpret_cast<value_type*>(block);
//             ++m_deep;
//           }
//           build(m_root, m_deep, num_block);
//         }
//       }

//       reference operator[](size_type index) {
//         if( m_tag==tag(index) )
//           return m_cache[jump(0, index)];
//         auto block = m_root;
//         for(rdeep lvl=m_deep; lvl>0; --lvl)
//           block = reinterpret_cast<value_type**>(block[jump(lvl, index)]);
//         m_tag = tag(index);
//         m_cache = reinterpret_cast<value_type*>(block);
//         return reinterpret_cast<value_type*>(block)[jump(0, index)];
//       }

// #ifdef RMV_DEBUG
//       friend std::ostream& operator<<(
//         std::ostream& stream, const rmv& vector) {
//         stream<<std::endl;
//         stream<<"deep  = "<<static_cast<size_type>(
//           vector.m_deep)<<std::endl;
//         stream<<"peek  = "<<static_cast<size_type>(
//           vector.m_peek)<<std::endl;
//         stream<<"tag   = "<<static_cast<size_type>(
//           vector.m_tag)<<std::endl;
//         stream<<"max   = "<<static_cast<size_type>(
//           vector.peek(vector.m_deep)+1)<<std::endl;
//         stream<<"root  = "<<static_cast<void*>(vector.m_root)<<std::endl;
//         stream<<"cache = "<<static_cast<void*>(vector.m_cache)<<std::endl;
//         stream<<std::endl;
//         vector.debug(const_cast<const_pointer*>(vector.m_root), 0, stream);
//         return stream;
//       }
// #endif
//   };
}
