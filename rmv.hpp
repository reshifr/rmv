#include <iostream>
#include <cinttypes>
#include <initializer_list>

namespace rsfr {

template <std::uint8_t E, class T>
class mv {
  public:
    using rlvldiff_type = std::int8_t;
    using rlvlsize_type = std::uint8_t;

#if SIZE_MAX == UINT64_MAX
    using rblockdiff_type = std::int32_t;
    using rblocksize_type = std::uint32_t;
#elif SIZE_MAX == UINT32_MAX
    using rblockdiff_type = std::int16_t;
    using rblocksize_type = std::uint16_t;
#endif

    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

  protected:
    size_type m_peek;
    pointer* m_root;
    rlvlsize_type m_deep;

    constexpr rblocksize_type block_size(void) const noexcept {
      return static_cast<rblocksize_type>(1)<<E; }
    constexpr rblocksize_type block_mask(void) const noexcept {
      return block_size()-1; }
    constexpr rblocksize_type block_count(size_type n) const noexcept {
      return (n>>E)+((n&block_mask())==0 ? 0 : 1); }
    constexpr size_type end(rlvlsize_type deep) const noexcept {
      return (static_cast<size_type>(1)<<(E*(deep+1)))-1; }
    constexpr size_type mask(rlvlsize_type lvl) const noexcept {
      return block_mask()<<(E*lvl); }
    constexpr rblocksize_type jump(rlvlsize_type lvl,
      size_type i) const noexcept { return (i&mask(lvl))>>(E*lvl); }

    mv(void) : m_peek(), m_root(), m_deep() {}
    ~mv(void) {}

    void fill(pointer* root, rlvlsize_type lvl, rblocksize_type& n) {
      if( lvl==1 ) {
        for(auto i=jump(lvl, m_peek+block_size());
            n!=0 && i<block_size(); ++i, --n, m_peek+=block_size())
          root[i] = new value_type[block_size()]();
        return;
      }
      for(auto i=jump(lvl, m_peek+block_size());
          n!=0 && i<block_size(); ++i) {
        if( root[i]==nullptr )
          root[i] = reinterpret_cast<pointer>(new pointer[block_size()]());
        fill(reinterpret_cast<pointer*>(root[i]), lvl-1, n);
      }
    }

    void build(rblocksize_type n) {
      if( m_peek==0 ) {
        m_root = reinterpret_cast<pointer*>(
          new value_type[block_size()]());
        m_peek = block_mask();
        --n;
      }
      while( n!=0 ) {
        if( m_peek==end(m_deep) ) {
          auto block = m_root;
          m_root = new pointer[block_size()]();
          m_root[0] = reinterpret_cast<pointer>(block);
          ++m_deep;
        }
        fill(m_root, m_deep, n);
      }
    }

    void push(pointer& cache) {
      if( m_peek==0 ) {
        m_root = reinterpret_cast<pointer*>(
          cache=new value_type[block_size()]());
        m_peek = block_mask();
        return;
      }
      auto block = m_root;
      if( m_peek==end(m_deep) ) {
        m_root = new pointer[block_size()]();
        m_root[0] = reinterpret_cast<pointer>(block);
        block = m_root;
        ++m_deep;
      }
      m_peek += block_size();
      for(auto lvl=m_deep; lvl>1; --lvl) {
        auto i = jump(lvl, m_peek);
        if( block[i]==nullptr )
          block[i] = reinterpret_cast<pointer>(new pointer[block_size()]());
        block = reinterpret_cast<pointer*>(block[i]);
      }
      block[jump(1, m_peek)] = cache = new value_type[block_size()]();
    }

  public:
    void debug(const_pointer* root, rlvlsize_type deep) const {
      if( root==nullptr )
        return;
      for(size_type i=0; i<(deep<<1); ++i)
        std::cout<<" ";
      std::cout<<"|";
      if( deep==m_deep ) {
        for(rblocksize_type i=0; i<block_size(); ++i)
          std::cout<<reinterpret_cast<const_pointer>(root)[i]<<"|";
        std::cout<<std::endl;
        return;
      }
      for(rblocksize_type i=0; i<block_size(); ++i)
        std::cout<<(static_cast<bool>(root[i]) ? "=" : " ")<<"|";
      std::cout<<std::endl;
      for(rblocksize_type i=0; i<block_size(); ++i)
        debug(reinterpret_cast<const_pointer*>(
          const_cast<pointer>(root[i])), deep+1);
    }
};

template <class V>
class mvi {
  public:
    using value_type = typename V::value_type;
    using reference = typename V::reference;
    using const_reference = typename V::const_reference;
    using pointer = typename V::pointer;
    using const_pointer = typename V::const_pointer;
    using size_type = typename V::size_type;
    using difference_type = typename V::difference_type;
    using iterator_category = std::random_access_iterator_tag;

  protected:
    difference_type m_pos;
    V* m_vector;

  public:
    mvi(void) : m_pos(), m_vector() {}
    mvi(V* vector) : m_pos(), m_vector(vector) {}
    mvi(V* vector, difference_type off) : m_pos(off), m_vector(vector) {}
    bool operator==(const mvi& it) const { return m_pos==it.m_pos; }
    bool operator!=(const mvi& it) const { return m_pos!=it.m_pos; }
    bool operator<(const mvi& it) const { return m_pos<it.m_pos; }
    bool operator<=(const mvi& it) const { return m_pos<=it.m_pos; }
    bool operator>(const mvi& it) const { return m_pos>it.m_pos; }
    bool operator>=(const mvi& it) const { return m_pos>=it.m_pos; }
};

template <class V>
class rmvi : public mvi<V> {
  using mvi<V>::m_pos;
  using mvi<V>::m_vector;

  public:
    using value_type = typename mvi<V>::value_type;
    using reference = typename mvi<V>::reference;
    using pointer = typename mvi<V>::pointer;
    using size_type = typename mvi<V>::size_type;
    using difference_type = typename mvi<V>::difference_type;
    using iterator_category = typename mvi<V>::iterator_category;

    rmvi(void) : mvi<V>() {}
    rmvi(V* vector) : mvi<V>(vector, 0) {}
    rmvi(V* vector, difference_type off) : mvi<V>(vector, off) {}

    reference operator*(void) const { return (*m_vector)[m_pos]; }
    pointer operator->(void) const { return &(*m_vector)[m_pos]; }
    reference operator[](difference_type off) const { return (*m_vector)[off]; }

    rmvi& operator++(void) { ++m_pos; return *this; }
    rmvi operator++(int) { return rmvi(m_vector, m_pos++); }
    rmvi& operator--(void) { --m_pos; return *this; }
    rmvi operator--(int) { return rmvi(m_vector, m_pos--); }

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

template <class V>
class rmvci : public mvi<V> {
  using mvi<V>::m_pos;
  using mvi<V>::m_vector;

  public:
    using value_type = typename mvi<V>::value_type;
    using const_reference = typename mvi<V>::const_reference;
    using const_pointer = typename mvi<V>::const_pointer;
    using size_type = typename mvi<V>::size_type;
    using difference_type = typename mvi<V>::difference_type;
    using iterator_category = typename mvi<V>::iterator_category;

    rmvci(void) : mvi<V>() {}
    rmvci(V* vector) : mvi<V>(vector, 0) {}
    rmvci(V* vector, difference_type off) : mvi<V>(vector, off) {}

    const_reference operator*(void) const {
      return (*m_vector)[m_pos]; }
    const_pointer operator->(void) const {
      return &(*m_vector)[m_pos]; }
    const_reference operator[](difference_type off) const {
      return (*m_vector)[off]; }

    rmvci& operator++(void) { ++m_pos; return *this; }
    rmvci operator++(int) { return rmvci(m_vector, m_pos++); }
    rmvci& operator--(void) { --m_pos; return *this; }
    rmvci operator--(int) { return rmvci(m_vector, m_pos--); }

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

template <class V>
class rmvri : public mvi<V> {
  using mvi<V>::m_pos;
  using mvi<V>::m_vector;

  public:
    using value_type = typename mvi<V>::value_type;
    using reference = typename mvi<V>::reference;
    using pointer = typename mvi<V>::pointer;
    using size_type = typename mvi<V>::size_type;
    using difference_type = typename mvi<V>::difference_type;
    using iterator_category = typename mvi<V>::iterator_category;

    rmvri(void) : mvi<V>() {}
    rmvri(V* vector) : mvi<V>(vector,
      static_cast<difference_type>(vector->size())-1) {}
    rmvri(V* vector, difference_type off) : mvi<V>(vector, off) {}

    reference operator*(void) const { return (*m_vector)[m_pos]; }
    pointer operator->(void) const { return &(*m_vector)[m_pos]; }
    reference operator[](difference_type off) const { return (*m_vector)[off]; }

    rmvri& operator++(void) { --m_pos; return *this; }
    rmvri operator++(int) { return rmvri(m_vector, m_pos--); }
    rmvri& operator--(void) { ++m_pos; return *this; }
    rmvri operator--(int) { return rmvri(m_vector, m_pos++); }

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

template <class V>
class rmvcri : public mvi<V> {
  using mvi<V>::m_pos;
  using mvi<V>::m_vector;

  public:
    using value_type = typename mvi<V>::value_type;
    using const_reference = typename mvi<V>::const_reference;
    using const_pointer = typename mvi<V>::const_pointer;
    using size_type = typename mvi<V>::size_type;
    using difference_type = typename mvi<V>::difference_type;
    using iterator_category = typename mvi<V>::iterator_category;

    rmvcri(void) : mvi<V>() {}
    rmvcri(V* vector) : mvi<V>(vector,
      static_cast<difference_type>(vector->size())-1) {}
    rmvcri(V* vector, difference_type off) : mvi<V>(vector, off) {}

    const_reference operator*(void) const {
      return (*m_vector)[m_pos]; }
    const_pointer operator->(void) const {
      return &(*m_vector)[m_pos]; }
    const_reference operator[](difference_type off) const {
      return (*m_vector)[off]; }

    rmvcri& operator++(void) { --m_pos; return *this; }
    rmvcri operator++(int) { return rmvcri(m_vector, m_pos--); }
    rmvcri& operator--(void) { ++m_pos; return *this; }
    rmvcri operator--(int) { return rmvcri(m_vector, m_pos++); }

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

template <std::uint8_t E, class T>
class rcmv : public mv<E, T> {
  using typename mv<E, T>::rlvldiff_type;
  using typename mv<E, T>::rlvlsize_type;
  using typename mv<E, T>::rblockdiff_type;
  using typename mv<E, T>::rblocksize_type;

  using mv<E, T>::m_peek;
  using mv<E, T>::m_root;
  using mv<E, T>::m_deep;
  using mv<E, T>::block_size;
  using mv<E, T>::block_mask;
  using mv<E, T>::block_count;
  using mv<E, T>::end;
  using mv<E, T>::mask;
  using mv<E, T>::jump;
  using mv<E, T>::fill;
  using mv<E, T>::build;
  using mv<E, T>::push;

  public:
    using value_type = typename mv<E, T>::value_type;
    using reference = typename mv<E, T>::reference;
    using const_reference = typename mv<E, T>::const_reference;
    using pointer = typename mv<E, T>::pointer;
    using const_pointer = typename mv<E, T>::const_pointer;
    using size_type = typename mv<E, T>::size_type;
    using difference_type = typename mv<E, T>::difference_type;
    using iterator = rmvi<rcmv<E, T>>;
    using const_iterator = rmvci<rcmv<E, T>>;
    using reverse_iterator = rmvri<rcmv<E, T>>;
    using const_reverse_iterator = rmvcri<rcmv<E, T>>;

  private:
    rblocksize_type m_free;

    pointer access_block(rblocksize_type index) {
      index <<= E;
      auto block = m_root;
      for(auto lvl=m_deep; lvl>0; --lvl)
        block = reinterpret_cast<pointer*>(block[jump(lvl, index)]);
      return reinterpret_cast<pointer>(block);
    }
  
  public:
    rcmv(void) : mv<E, T>(), m_free() {}
    rcmv(size_type num) : rcmv() { extend(num); }
    rcmv(const std::initializer_list<value_type>& init) : rcmv() {
      extend(init.size());
      auto num_block = init.size()>>E;
      rblocksize_type i, index;
      for(i=0; i<num_block; ++i) {
        index = 0;
        auto block = access_block(i);
        auto end = init.begin()+(i<<E)+block_size();
        for(auto p_elm=init.begin()+(i<<E); p_elm!=end; ++p_elm, ++index)
          block[index] = *p_elm;
      }
      if( (init.size()&block_mask())==0 )
        return;
      index = 0;
      auto block = access_block(i);
      auto end = init.begin()+(i<<E)+(init.size()&block_mask());
      for(auto p_elm=init.begin()+(i<<E); p_elm!=end; ++p_elm, ++index)
        block[index] = *p_elm;
    }

    void extend(size_type num) {
      auto new_size = size()+num;
      if( m_free>num ) {
        m_free -= num;
        return;
      } else
        num -= m_free;
      auto num_block = block_count(num);
      build(num_block);
      m_free = capacity()-new_size;
    }

    void push_back(const reference val) {
      if( m_free>0 ) {
        operator[](m_peek-(--m_free)) = val;
        return;
      }
      pointer cache;
      push(cache);
      cache[0] = val;
      m_free = block_mask();
    }

    void push_back(const value_type&& val) {
      if( m_free>0 ) {
        operator[](m_peek-(--m_free)) = val;
        return;
      }
      pointer cache;
      push(cache);
      cache[0] = val;
      m_free = block_mask();
    }

    reference operator[](size_type index) {
      auto block = m_root;
      for(auto lvl=m_deep; lvl>0; --lvl)
        block = reinterpret_cast<pointer*>(block[jump(lvl, index)]);
      return reinterpret_cast<pointer>(block)[jump(0, index)];
    }

    const_reference operator[](size_type index) const {
      auto block = m_root;
      for(auto lvl=m_deep; lvl>0; --lvl)
        block = reinterpret_cast<pointer*>(block[jump(lvl, index)]);
      return reinterpret_cast<pointer>(block)[jump(0, index)];
    }

    size_type size(void) const noexcept {
      return capacity()-m_free; }
    size_type capacity(void) const noexcept {
      return m_root==nullptr ? 0 : m_peek+1; }

    iterator begin(void) noexcept {
      return iterator(this); }
    const_iterator begin(void) const noexcept {
      return const_iterator(const_cast<rcmv<E, T>*>(this)); }
    const_iterator cbegin(void) const noexcept {
      return const_iterator(const_cast<rcmv<E, T>*>(this)); }

    iterator end(void) noexcept {
      return iterator(this, size()); }
    const_iterator end(void) const noexcept {
      return const_iterator(const_cast<rcmv<E, T>*>(this), size()); }
    const_iterator cend(void) const noexcept {
      return const_iterator(const_cast<rcmv<E, T>*>(this), size()); }

    reverse_iterator rbegin(void) noexcept {
      return reverse_iterator(this); }
    const_reverse_iterator rbegin(void) const noexcept {
      return const_reverse_iterator(const_cast<rcmv<E, T>*>(this)); }
    const_reverse_iterator crbegin(void) const noexcept {
      return const_reverse_iterator(const_cast<rcmv<E, T>*>(this)); }
    reverse_iterator rend(void) noexcept {
      return reverse_iterator(this, -1); }
    const_reverse_iterator rend(void) const noexcept {
      return const_reverse_iterator(const_cast<rcmv<E, T>*>(this), -1); }
    const_reverse_iterator crend(void) const noexcept {
      return const_reverse_iterator(const_cast<rcmv<E, T>*>(this), -1); }

  public:
    void debug(void) const {
      std::cout<<std::endl;
      std::cout<<"deep     = "<<static_cast<size_type>(m_deep)<<std::endl;
      std::cout<<"free     = "<<static_cast<size_type>(m_free)<<std::endl;
      std::cout<<"peek     = "<<static_cast<size_type>(m_peek)<<std::endl;
      std::cout<<"size     = "<<static_cast<size_type>(size())<<std::endl;
      std::cout<<"capacity = "<<static_cast<size_type>(capacity())<<std::endl;
      std::cout<<"max      = "<<
        static_cast<size_type>(end(m_deep)+1)<<std::endl;
      std::cout<<"root     = "<<
        static_cast<void*>(m_root)<<std::endl<<std::endl;
#ifndef RMV_DEBUG
      mv<E, T>::debug(const_cast<const_pointer*>(m_root), 0);
#endif
      std::cout<<std::endl;
    }
};

}
