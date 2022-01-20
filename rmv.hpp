/**
 * Reshifr Multilevel Vector
 * Copyright (c) 2021 Renol P. H. <reshifr@gmail.com>
 * 
 * MIT License
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall
 * be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef RSFR_RMV_H
#define RSFR_RMV_H

#include <limits>
#include <iostream>
#include <type_traits>
#include <initializer_list>

namespace rsfr {

template <std::uint8_t Exp, class Tp>
class mv {
  public:
    using value_type = Tp;
    using reference = Tp&;
    using const_reference = const Tp&;
    using pointer = Tp*;
    using const_pointer = const Tp*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using mvlsize_type = std::uint8_t;
    using mvldiff_type = std::int8_t;
    using mvbsize_type = typename std::conditional<
      std::numeric_limits<size_type>::digits==
      std::numeric_limits<std::uint64_t>::digits,
      std::uint32_t, std::uint16_t>::type;
    using mvbdiff_type = typename std::conditional<
      std::numeric_limits<difference_type>::digits==
      std::numeric_limits<std::int64_t>::digits,
      std::int32_t, std::int16_t>::type;

  protected:
    struct mvb {
      template <class Blk>
      static void dlloc(Blk block)
        { delete[] block; }

      struct val {
        static Tp* alloc(void)
          { return new Tp[size()]; }
        static Tp* alloc(const Tp& val) {
          auto block = new Tp[size()];
          std::fill_n(block, size(), val);
          return block;
        }
      };

      struct index {
        static Tp** alloc(void)
          { return new Tp*[size()](); }
      };
      
      static consteval mvbsize_type
        size(void) noexcept
        { return static_cast<mvbsize_type>(1)<<Exp; }
      static consteval mvbsize_type
        mask(void) noexcept
        { return size()-1; }
      static constexpr size_type
        end(mvlsize_type deep) noexcept
        { return (static_cast<size_type>(1)<<(Exp*(deep+1)))-1; }
      static constexpr mvbsize_type
        jump(mvlsize_type lvl, size_type i) noexcept
        { return static_cast<mvbsize_type>(i&mask()<<(Exp*lvl))>>(Exp*lvl); }
    };

    union mvp {
      Tp* val;
      Tp** index;
      Tp*** pindex;
    } m_root;
    size_type m_peek;
    mvlsize_type m_deep;

    mv(void) noexcept : m_root(), m_peek(), m_deep() {}
    ~mv(void) { destroy(); }

  private:
    /**
     * @brief   Destroy tree.
     * 
     * Leaving only the block of index.
     * 
     * @param   root   Root union
     * @param   lvl    Level of tree
     */
    void recursive_del(mvp root, mvlsize_type lvl) {
      if( lvl==1 ) {
        // dealloc block of value
        for(mvbdiff_type i=mvb::jump(lvl, m_peek);
            i>=0; --i, m_peek-=mvb::size())
          mvb::dlloc(root.index[i]);
        return;
      }
      // dealloc block of index
      for(mvbdiff_type i=mvb::jump(lvl, m_peek); i>=0; --i) {
        recursive_del({.index=root.pindex[i]}, lvl-1);
        mvb::dlloc(root.pindex[i]);
      }
    }

    /**
     * @brief   Fill tree.
     * 
     * Just fills the tree with blocks without initialization
     * value, without increasing the height.
     * 
     * @param   root   Root union
     * @param   lvl    Level of tree
     * @param   n      Num of blocks
     */
    void recursive_fill(mvp root,
      mvlsize_type lvl, mvbsize_type& n) {
      if( lvl==1 ) {
        // alloc block of value
        for(auto i=mvb::jump(lvl, m_peek+mvb::size());
            n!=0 && i<mvb::size(); ++i, --n, m_peek+=mvb::size())
          root.index[i] = mvb::val::alloc();
        return;
      }
      // alloc block of index
      for(auto i=mvb::jump(lvl, m_peek+mvb::size());
          n!=0 && i<mvb::size(); ++i) {
        if( root.pindex[i]==nullptr )
          root.pindex[i] = mvb::index::alloc();
        recursive_fill({.index=root.pindex[i]}, lvl-1, n);
      }
    }

    /**
     * @brief   Fill tree.
     * 
     * Just fills the tree with blocks with initialization value,
     * without increasing the height.
     * 
     * @param   root   Root union
     * @param   lvl    Level of tree
     * @param   n      Num of blocks
     * @param   val    Initialization value
     */
    void recursive_fill(mvp root,
      mvlsize_type lvl, mvbsize_type& n, const Tp& val) {
      if( lvl==1 ) {
        // alloc block of value
        for(auto i=mvb::jump(lvl, m_peek+mvb::size());
            n!=0 && i<mvb::size(); ++i, --n, m_peek+=mvb::size())
          root.index[i] = mvb::val::alloc(val);
        return;
      }
      // alloc block of index
      for(auto i=mvb::jump(lvl, m_peek+mvb::size());
          n!=0 && i<mvb::size(); ++i) {
        if( root.pindex[i]==nullptr )
          root.pindex[i] = mvb::index::alloc();
        recursive_fill({.index=root.pindex[i]}, lvl-1, n, val);
      }
    }

    /**
     * @brief   Reduce tree.
     * 
     * @param   root   Root union
     * @param   lvl    Level of tree
     * @param   n      Num of blocks
     * 
     * @return  - True, the tree must be destroyed, leaving only
     *            the block of index.
     *          - False, the tree is reduced but height is not
     *            reduced.
     */
    bool recursive_reduce(mvp root,
      mvlsize_type lvl, mvbsize_type& n) {
      if( lvl==1 ) {
        mvbdiff_type i=mvb::jump(lvl, m_peek);
        // dealloc block of value
        for(; n!=0 && i>=0; --i, --n, m_peek-=mvb::size()) {
          mvb::dlloc(root.index[i]);
          root.index[i] = nullptr;
        }
        return i<0;
      }
      mvbdiff_type i=mvb::jump(lvl, m_peek);
      // dealloc block of index
      for(; i>=0; --i) {
        if( !recursive_reduce({.index=root.pindex[i]}, lvl-1, n) )
          return false;
        mvb::dlloc(root.pindex[i]);
        root.pindex[i] = nullptr;
      }
      return i<0;
    }

  protected:
    void fill(mvbsize_type n) {
      if( n==0 )
        return;
      // if the tree is empty, alloc block of value as root
      if( m_peek==0 ) {
        m_root.val = mvb::val::alloc();
        m_peek = mvb::mask();
        --n;
      }
      while( n!=0 ) {
        // if the tree is not fit,
        // increase the tree height with alloc block of index
        if( m_peek==mvb::end(m_deep) ) {
          auto block = m_root;
          m_root.index = mvb::index::alloc();
          m_root.pindex[0] = block.index;
          ++m_deep;
        }
        // fill the tree with blocks
        recursive_fill(m_root, m_deep, n);
      }
    }

    void fill(mvbsize_type n, const Tp& val) {
      if( n==0 )
        return;
      // if the tree is empty, alloc block of value as root
      if( m_peek==0 ) {
        m_root.val = mvb::val::alloc(val);
        m_peek = mvb::mask();
        --n;
      }
      while( n!=0 ) {
        // if the tree is not fit,
        // increase the tree height with alloc block of index
        if( m_peek==mvb::end(m_deep) ) {
          auto block = m_root;
          m_root.index = mvb::index::alloc();
          m_root.pindex[0] = block.index;
          ++m_deep;
        }
        // fill the tree with blocks
        recursive_fill(m_root, m_deep, n, val);
      }
    }
    
    Tp* push(void) {
      // if the tree is empty, alloc block of value as root
      if( m_peek==0 ) {
        m_root.val = mvb::val::alloc();
        m_peek = mvb::mask();
        return m_root.val;
      }
      auto block = m_root;
      // if the tree is not enough,
      // increase the height with alloc block of index
      if( m_peek==mvb::end(m_deep) ) {
        m_root.index = mvb::index::alloc();
        m_root.pindex[0] = block.index;
        block = m_root;
        ++m_deep;
      }
      m_peek += mvb::size();
      // fill the block of index with alloc block of index
      for(auto lvl=m_deep; lvl>1; --lvl) {
        auto i = mvb::jump(lvl, m_peek);
        if( block.pindex[i]==nullptr )
          block.pindex[i] = mvb::index::alloc();
        block.index = block.pindex[i];
      }
      // alloc block of value
      return block.index[mvb::jump(1, m_peek)]=mvb::val::alloc();
    }

    Tp* tail(void) noexcept {
      // if the tree is empty
      if( m_peek==0 )
        return nullptr;
      auto block = m_root;
      for(auto lvl=m_deep; lvl>0; --lvl)
        block.index = block.pindex[mvb::jump(lvl, m_peek)];
      return block.val;
    }

    void reduce(mvbsize_type n) {
      if( n==0 || m_peek==0 )
        return;
      // if only block of value, dealloc block of value
      if( m_deep==0 ) {
        mvb::dlloc(m_root.val);
        m_root.val = nullptr;
        m_peek = 0;
        return;
      }
      // if the tree need to be destroyed, dealloc block of index
      if( recursive_reduce(m_root, m_deep, n) ) {
        mvb::dlloc(m_root.index);
        m_root.index = nullptr;
        m_peek = m_deep = 0;
        return;
      }
      // if the tree doesn't need to be destroyed,
      // reduce the height with dealloc block of index
      while( m_peek<=mvb::end(m_deep-1) ) {
        auto block = m_root;
        m_root.index = m_root.pindex[0];
        mvb::dlloc(block.index);
        if( --m_deep==0 )
          return;
      }
    }

    void destroy(void) {
      if( m_peek==0 )
        return;
      // dealloc block of value
      if( m_deep==0 ) {
        mvb::dlloc(m_root.val);
        m_root.val = nullptr;
        m_peek = 0;
        return;
      }
      // destroy the tree
      recursive_del(m_root, m_deep);
      mvb::dlloc(m_root.index);
      m_root.index = nullptr;
      m_peek = m_deep = 0;
    }

  public:
    void debug(mvp root, mvlsize_type deep) const {
      if( root.val==nullptr )
        return;
      for(size_type i=0; i<(size_type)(deep<<1); ++i)
        std::cout<<" ";
      std::cout<<"|";
      if( deep==m_deep ) {
        // iterating block of value
        for(mvbsize_type i=0; i<mvb::size(); ++i)
          std::cout<<root.val[i]<<"|";
        std::cout<<std::endl;
        return;
      }
      // iterating block of index
      for(mvbsize_type i=0; i<mvb::size(); ++i)
        std::cout<<(static_cast<bool>(root.pindex[i]) ? "=" : " ")<<"|";
      std::cout<<std::endl;
      // block of index call each element
      for(mvbsize_type i=0; i<mvb::size(); ++i)
        debug({.index=root.pindex[i]}, deep+1);
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

  public:
    mvi(void) noexcept : m_pos() {}
    mvi(difference_type off) noexcept : m_pos(off) {}
    bool operator==(const mvi& it) const noexcept { return m_pos==it.m_pos; }
    bool operator!=(const mvi& it) const noexcept { return m_pos!=it.m_pos; }
    bool operator<(const mvi& it) const noexcept { return m_pos<it.m_pos; }
    bool operator<=(const mvi& it) const noexcept { return m_pos<=it.m_pos; }
    bool operator>(const mvi& it) const noexcept { return m_pos>it.m_pos; }
    bool operator>=(const mvi& it) const noexcept { return m_pos>=it.m_pos; }
};

template <class V>
class rmvi : public mvi<V> {
  using mvi<V>::m_pos;

  public:
    using value_type = typename mvi<V>::value_type;
    using reference = typename mvi<V>::reference;
    using pointer = typename mvi<V>::pointer;
    using size_type = typename mvi<V>::size_type;
    using difference_type = typename mvi<V>::difference_type;
    using iterator_category = typename mvi<V>::iterator_category;

  private:
    V* m_vector;

  public:
    rmvi(void) noexcept = default;
    rmvi(V* vector) noexcept : m_vector(vector) {}
    rmvi(V* vector, difference_type off) noexcept :
      mvi<V>(off), m_vector(vector) {}

    reference operator*(void) const noexcept
      { return (*m_vector)[m_pos]; }
    pointer operator->(void) const noexcept
      { return &(*m_vector)[m_pos]; }
    reference operator[](difference_type off) const noexcept
      { return (*m_vector)[off]; }

    rmvi& operator++(void) noexcept { ++m_pos; return *this; }
    rmvi operator++(int) noexcept { return rmvi(m_vector, m_pos++); }
    rmvi& operator--(void) noexcept { --m_pos; return *this; }
    rmvi operator--(int) noexcept { return rmvi(m_vector, m_pos--); }

    rmvi& operator+=(difference_type off) noexcept
      { m_pos += off; return *this; }
    rmvi operator+(difference_type off) const noexcept
      { return rmvi(m_vector, m_pos+off); }
    friend rmvi operator+(difference_type off, const rmvi& it) noexcept
      { return rmvi(it.m_vector, off+it.m_pos); }
    rmvi& operator-=(difference_type off) noexcept
      { m_pos -= off; return *this; }
    rmvi operator-(difference_type off) const noexcept
      { return rmvi(m_vector, m_pos-off); }
    difference_type operator-(const rmvi& it) const noexcept
      { return m_pos-it.m_pos; }
};

template <class V>
class rmvci : public mvi<V> {
  using mvi<V>::m_pos;

  public:
    using value_type = typename mvi<V>::value_type;
    using reference = typename mvi<V>::const_reference;
    using pointer = typename mvi<V>::const_pointer;
    using size_type = typename mvi<V>::size_type;
    using difference_type = typename mvi<V>::difference_type;
    using iterator_category = typename mvi<V>::iterator_category;

  private:
    const V* m_vector;

  public:
    rmvci(void) noexcept = default;
    rmvci(const V* vector) noexcept : m_vector(vector) {}
    rmvci(const V* vector, difference_type off) noexcept :
      mvi<V>(off), m_vector(vector) {}

    reference operator*(void) const noexcept
      { return (*m_vector)[m_pos]; }
    pointer operator->(void) const noexcept
      { return &(*m_vector)[m_pos]; }
    reference operator[](difference_type off) const noexcept
      { return (*m_vector)[off]; }

    rmvci& operator++(void) noexcept { ++m_pos; return *this; }
    rmvci operator++(int) noexcept { return rmvci(m_vector, m_pos++); }
    rmvci& operator--(void) noexcept { --m_pos; return *this; }
    rmvci operator--(int) noexcept { return rmvci(m_vector, m_pos--); }

    rmvci& operator+=(difference_type off) noexcept
      { m_pos += off; return *this; }
    rmvci operator+(difference_type off) const noexcept
      { return rmvci(m_vector, m_pos+off); }
    friend rmvci operator+(difference_type off, const rmvci& it) noexcept
      { return rmvci(it.m_vector, off+it.m_pos); }
    rmvci& operator-=(difference_type off) noexcept
      { m_pos -= off; return *this; }
    rmvci operator-(difference_type off) const noexcept
      { return rmvci(m_vector, m_pos-off); }
    difference_type operator-(const rmvci& it) const noexcept
      { return m_pos-it.m_pos; }
};

template <std::uint8_t Exp, class Tp>
class rpmv : protected mv<Exp, Tp> {
  using typename mv<Exp, Tp>::mvlsize_type;
  using typename mv<Exp, Tp>::mvldiff_type;
  using typename mv<Exp, Tp>::mvbsize_type;
  using typename mv<Exp, Tp>::mvbdiff_type;

  using mv<Exp, Tp>::m_peek;
  using mv<Exp, Tp>::m_root;
  using mv<Exp, Tp>::m_deep;
  using mv<Exp, Tp>::fill;
  using mv<Exp, Tp>::push;
  using mv<Exp, Tp>::tail;
  using mv<Exp, Tp>::reduce;
  using mv<Exp, Tp>::destroy;

  public:
    using value_type = typename mv<Exp, Tp>::value_type;
    using reference = typename mv<Exp, Tp>::reference;
    using const_reference = typename mv<Exp, Tp>::const_reference;
    using pointer = typename mv<Exp, Tp>::pointer;
    using const_pointer = typename mv<Exp, Tp>::const_pointer;
    using size_type = typename mv<Exp, Tp>::size_type;
    using difference_type = typename mv<Exp, Tp>::difference_type;
    using iterator = rmvi<rpmv<Exp, Tp>>;
    using const_iterator = rmvci<rpmv<Exp, Tp>>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  private:
    mvbsize_type m_free;
    struct mvb : public mv<Exp, Tp>::mvb {};
  
  public:

    // void contoh(void) {
    //   mvb::dlloc(new int[1]);
    // }



    rpmv(void) noexcept : m_free() {}
    rpmv(size_type num) : rpmv() { extend(num); }

    template <class InIt>
    rpmv(InIt first, InIt last) : rpmv() {}

    void assign(size_type num, const Tp& val);

    void clear(void) {
      destroy();
      m_free = 0;
    }
  
  private:
    template <class Val>
    void elm_push(Val val) {
      if( m_free>0 ) {
        (*this)[m_peek-(--m_free)] = std::forward<Val>(val);
        return;
      }
      push()[0] = std::forward<Val>(val);
      m_free = mvb::mask();
    }

  public:
    void push_back(const Tp& val) { elm_push(val); }
    void push_back(Tp&& val) { elm_push(std::move(val)); }

    void extend(size_type num) {
      auto new_size = size()+num;
      if( m_free>num ) {
        m_free -= num;
        return;
      }
      num -= m_free;
      mvbsize_type num_block = (num>>Exp)+((num&mvb::mask())!=0);
      fill(num_block);
      m_free = capacity()-new_size;
    }

    void extend(size_type num, const Tp& val) {
      auto new_size = size()+num;
      if( m_free>num ) {
        std::fill_n(tail()+(mvb::size()-m_free), num, val);
        m_free -= num;
        return;
      }
      std::fill_n(tail()+(mvb::size()-m_free), m_free, val);
      num -= m_free;
      mvbsize_type num_block = num>>Exp;
      fill(num_block, val);
      if( (num&mvb::mask())!=0 )
        std::fill_n(push(), num&mvb::mask(), val);
      m_free = capacity()-new_size;
    }

    void shrink(size_type num) {
      auto new_size = size()-num;
      if( mvb::size()-m_free>num ) {
        m_free += num;

        std::cout<<"ERROR!"<<std::endl;
        return;
      }
      mvbsize_type num_block = num>>Exp;
      reduce(num_block);
      m_free = capacity()-new_size;
    }

  private:
    reference elm_access(size_type index) const noexcept {
      auto block = m_root;
      for(auto lvl=m_deep; lvl>0; --lvl)
        block.index = block.pindex[mvb::jump(lvl, index)];
      return block.val[mvb::jump(0, index)];
    }

  public:
    reference operator[](size_type index) noexcept
      { return elm_access(index); }
    const_reference operator[](size_type index) const noexcept 
      { return elm_access(index); }

    // reference front(void) noexcept {
    //   return elm_access(m_)
    // }

    bool empty(void) const noexcept
      { return m_peek==0; }
    size_type size(void) const noexcept
      { return capacity()-m_free; }
    static consteval size_type max_size(void) noexcept
      { return static_cast<size_type>(1)<<
        (std::numeric_limits<size_type>::digits-2); }
    static consteval size_type max_exponent(void) noexcept
      { return std::numeric_limits<mvbsize_type>::digits-2; }
    size_type capacity(void) const noexcept
      { return m_peek+(m_peek!=0); }

    iterator begin(void) noexcept
      { return iterator(this); }
    const_iterator begin(void) const noexcept
      { return cbegin(); }
    const_iterator cbegin(void) const noexcept
      { return const_iterator(this); }
    iterator end(void) noexcept
      { return iterator(this, size()); }
    const_iterator end(void) const noexcept
      { return cend(); }
    const_iterator cend(void) const noexcept
      { return const_iterator(this, size()); }

    reverse_iterator rbegin(void) noexcept
      { return reverse_iterator(end()); }
    const_reverse_iterator rbegin(void) const noexcept
      { return crbegin(); }
    const_reverse_iterator crbegin(void) const noexcept
      { return const_reverse_iterator(cend()); }
    reverse_iterator rend(void) noexcept
      { return reverse_iterator(begin()); }
    const_reverse_iterator rend(void) const noexcept
      { return crend(); }
    const_reverse_iterator crend(void) const noexcept
      { return const_reverse_iterator(cbegin()); }

  public:
    void debug(void) const {
      std::cout<<std::endl;
      std::cout<<"deep     = "<<(size_type)m_deep<<std::endl;
      std::cout<<"free     = "<<(size_type)m_free<<std::endl;
      std::cout<<"peek     = "<<(size_type)m_peek<<std::endl;
      std::cout<<"size     = "<<(size_type)size()<<std::endl;
      std::cout<<"capacity = "<<(size_type)capacity()<<std::endl;
      std::cout<<"max      = "<<(size_type)(mvb::end(m_deep)+1)<<std::endl;
      std::cout<<"root     = "<<m_root.val<<std::endl<<std::endl;
#ifdef RMV_DEBUG
      mv<Exp, Tp>::debug(m_root, 0);
#endif
      std::cout<<std::endl;
    }
};

}

#endif /* RSFR_RMV_H */
