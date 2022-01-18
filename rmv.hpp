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

    using lvldiff_type = std::int8_t;
    using lvlsize_type = std::uint8_t;
    using blocksize_type = typename std::conditional<
      std::numeric_limits<size_type>::max()==
      std::numeric_limits<std::uint64_t>::max(),
      std::uint32_t, std::uint16_t>::type;
    using blockdiff_type = typename std::conditional<
      std::numeric_limits<difference_type>::max()==
      std::numeric_limits<std::int64_t>::max(),
      std::int32_t, std::int16_t>::type;

  private:
    template <class Blk>
    static void block_dlloc(Blk block) { delete[] block; }
    template <class Elm>
    static Elm* block_alloc(void) { return new Elm[block_size()](); }

  protected:
    size_type m_peek;
    union mvb {
      Tp* val;
      Tp** index;
      Tp*** pindex;
    } m_root;
    lvlsize_type m_deep;

    static consteval blocksize_type
      block_size(void) noexcept
      { return static_cast<blocksize_type>(1)<<Exp; }
    static consteval blocksize_type
      block_mask(void) noexcept
      { return block_size()-1; }
    static constexpr size_type
      end(lvlsize_type deep) noexcept
      { return (static_cast<size_type>(1)<<(Exp*(deep+1)))-1; }
    static constexpr size_type
      mask(lvlsize_type lvl) noexcept
      { return block_mask()<<(Exp*lvl); }
    static constexpr blocksize_type
      jump(lvlsize_type lvl, size_type i) noexcept
      { return static_cast<blocksize_type>(i&mask(lvl))>>(Exp*lvl); }

    mv(void) noexcept :
      m_peek(),
      m_root(),
      m_deep() {}

    ~mv(void) {
      if( m_peek==0 )
        return;
      if( m_deep==0 ) {
        block_dlloc(m_root.val);
        return;
      }
      recursive_del(m_root, m_deep);
      block_dlloc(m_root.index);
    }

  private:
    /**
     * @brief   Destroy tree.
     * 
     * Leaving only the block of index.
     * 
     * @param   root   Root union
     * @param   lvl    Level of tree
     */
    void recursive_del(mvb root, lvlsize_type lvl) {
      if( lvl==1 ) {
        // dealloc block of value
        for(blockdiff_type i=jump(lvl, m_peek);
            i>=0; --i, m_peek-=block_size())
          block_dlloc(root.index[i]);
        return;
      }
      // dealloc block of index
      for(blockdiff_type i=jump(lvl, m_peek); i>=0; --i) {
        recursive_del({.index=root.pindex[i]}, lvl-1);
        block_dlloc(root.pindex[i]);
      }
    }

    /**
     * @brief   Fill tree.
     * 
     * Just fills the tree with blocks, without increasing the
     * height.
     * 
     * @param   root   Root union
     * @param   lvl    Level of tree
     * @param   n      Num of blocks
     */
    void recursive_fill(mvb root,
      lvlsize_type lvl, blocksize_type& n) {
      if( lvl==1 ) {
        // alloc block of value
        for(auto i=jump(lvl, m_peek+block_size());
            n!=0 && i<block_size(); ++i, --n, m_peek+=block_size())
          root.index[i] = block_alloc<Tp>();
        return;
      }
      // alloc block of index
      for(auto i=jump(lvl, m_peek+block_size());
          n!=0 && i<block_size(); ++i) {
        if( root.pindex[i]==nullptr )
          root.pindex[i] = block_alloc<Tp*>();
        recursive_fill({.index=root.pindex[i]}, lvl-1, n);
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
    bool recursive_reduce(mvb root,
      lvlsize_type lvl, blocksize_type& n) {
      if( lvl==1 ) {
        blockdiff_type i=jump(lvl, m_peek);
        // dealloc block of value
        for(; n!=0 && i>=0; --i, --n, m_peek-=block_size()) {
          block_dlloc(root.index[i]);
          root.index[i] = nullptr;
        }
        return i<0;
      }
      blockdiff_type i=jump(lvl, m_peek);
      // dealloc block of index
      for(; i>=0; --i) {
        if( !recursive_reduce({.index=root.pindex[i]}, lvl-1, n) )
          return false;
        block_dlloc(root.pindex[i]);
        root.pindex[i] = nullptr;
      }
      return i<0;
    }

  protected:
    void fill(blocksize_type n) {
      // if the tree is empty, alloc block of value as root
      if( m_peek==0 ) {
        m_root.val = block_alloc<Tp>();
        m_peek = block_mask();
        --n;
      }
      while( n!=0 ) {
        // if the tree is not fit,
        // increase the tree height with alloc block of index
        if( m_peek==end(m_deep) ) {
          auto block = m_root;
          m_root.index = block_alloc<Tp*>();
          m_root.pindex[0] = block.index;
          ++m_deep;
        }
        // fill the tree with blocks
        recursive_fill(m_root, m_deep, n);
      }
    }
    
    Tp* push(void) {
      // if the tree is empty, alloc block of value as root
      if( m_peek==0 ) {
        m_root.val = block_alloc<Tp>();
        m_peek = block_mask();
        return m_root.val;
      }
      auto block = m_root;
      // if the tree is not enough,
      // increase the height with alloc block of index
      if( m_peek==end(m_deep) ) {
        m_root.index = block_alloc<Tp*>();
        m_root.pindex[0] = block.index;
        block = m_root;
        ++m_deep;
      }
      m_peek += block_size();
      // fill the block of index with alloc block of index
      for(auto lvl=m_deep; lvl>1; --lvl) {
        auto i = jump(lvl, m_peek);
        if( block.pindex[i]==nullptr )
          block.pindex[i] = block_alloc<Tp*>();
        block.index = block.pindex[i];
      }
      // alloc block of value
      return block.index[jump(1, m_peek)]=block_alloc<Tp>();
    }

    void reduce(blocksize_type n) {
      if( n==0 || m_peek==0 )
        return;
      // if only block of value, dealloc block of value
      if( m_deep==0 ) {
        block_dlloc(m_root.val);
        m_root.val = nullptr;
        m_peek = 0;
        return;
      }
      // if the tree need to be destroyed, dealloc block of index
      if( recursive_reduce(m_root, m_deep, n) ) {
        block_dlloc(m_root.index);
        m_root.index = nullptr;
        m_peek = m_deep = 0;
        return;
      }
      // if the tree doesn't need to be destroyed,
      // reduce the height with dealloc block of index
      while( m_peek<=end(m_deep-1) ) {
        auto block = m_root;
        m_root.index = m_root.pindex[0];
        block_dlloc(block.index);
        if( --m_deep==0 )
          return;
      }
    }

  public:
    void debug(mvb root, lvlsize_type deep) const {
      if( root.val==nullptr )
        return;
      for(size_type i=0; i<(size_type)(deep<<1); ++i)
        std::cout<<" ";
      std::cout<<"|";
      if( deep==m_deep ) {
        // iterating block of value
        for(blocksize_type i=0; i<block_size(); ++i)
          std::cout<<root.val[i]<<"|";
        std::cout<<std::endl;
        return;
      }
      // iterating block of index
      for(blocksize_type i=0; i<block_size(); ++i)
        std::cout<<(static_cast<bool>(root.pindex[i]) ? "=" : " ")<<"|";
      std::cout<<std::endl;
      // block of index call each element
      for(blocksize_type i=0; i<block_size(); ++i)
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
  using typename mv<Exp, Tp>::lvldiff_type;
  using typename mv<Exp, Tp>::lvlsize_type;
  using typename mv<Exp, Tp>::blockdiff_type;
  using typename mv<Exp, Tp>::blocksize_type;

  using mv<Exp, Tp>::m_peek;
  using mv<Exp, Tp>::m_root;
  using mv<Exp, Tp>::m_deep;
  using mv<Exp, Tp>::block_size;
  using mv<Exp, Tp>::block_mask;
  using mv<Exp, Tp>::end;
  using mv<Exp, Tp>::mask;
  using mv<Exp, Tp>::jump;
  using mv<Exp, Tp>::fill;
  using mv<Exp, Tp>::push;
  using mv<Exp, Tp>::reduce;

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
    blocksize_type m_free;
  
  public:
    rpmv(void) noexcept : m_free() {}
    rpmv(size_type num) : rpmv() { extend(num); }

    template <class InIt>
    rpmv(InIt first, InIt last) : rpmv() {
      return;
    }

    // Modifiers

    void push_back(const Tp& val) {
      // std::cout<<"const&"<<std::endl;
      if( m_free>0 ) {
        (*this)[m_peek-(--m_free)] = val;
        return;
      }
      auto cache = push();
      cache[0] = val;
      m_free = block_mask();
    }

    void push_back(Tp&& val) {
      // std::cout<<"&&"<<std::endl;
      if( m_free>0 ) {
        (*this)[m_peek-(--m_free)] = std::move(val);
        return;
      }
      auto cache = push();
      cache[0] = std::move(val);
      m_free = block_mask();
    }

    void extend(size_type num) {
      auto new_size = size()+num;
      if( m_free>num ) {
        m_free -= num;
        return;
      } else
        num -= m_free;
      blocksize_type num_block = (num>>Exp)+((num&block_mask())==0 ? 0 : 1);
      fill(num_block);
      m_free = capacity()-new_size;
    }

    // Element Access

    reference operator[](size_type index) noexcept {
      auto block = m_root;
      for(auto lvl=m_deep; lvl>0; --lvl)
        block.index = block.pindex[jump(lvl, index)];
      return block.val[jump(0, index)];
    }

    const_reference operator[](size_type index) const noexcept {
      auto block = m_root;
      for(auto lvl=m_deep; lvl>0; --lvl)
        block.index = block.pindex[jump(lvl, index)];
      return block.val[jump(0, index)];
    }

    // Capacity

    size_type size(void) const noexcept
      { return capacity()-m_free; }
    size_type capacity(void) const noexcept
      { return m_root.val==nullptr ? 0 : m_peek+1; }

    // Iterators

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
      std::cout<<"max      = "<<(size_type)(end(m_deep)+1)<<std::endl;
      std::cout<<"root     = "<<m_root.val<<std::endl<<std::endl;
#ifndef RMV_DEBUG
      mv<Exp, Tp>::debug(m_root, 0);
#endif
      std::cout<<std::endl;
    }
};

}

#endif /* RSFR_RMV_H */
