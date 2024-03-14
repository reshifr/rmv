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
#include <cstdint>
#include <iostream>
#include <functional>
#include <type_traits>
#include <initializer_list>

namespace rsfr {

template <std::uint8_t Exp, class Tp>
struct mvb {
  using value_type = Tp;
  using reference = Tp&;
  using const_reference = const Tp&;
  using pointer = Tp*;
  using const_pointer = const Tp*;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using mvlsize_type = std::uint8_t;
  using mvldiff_type = std::int8_t;
  using mvbsize_type = typename std::conditional_t<
    std::numeric_limits<size_type>::digits==
    std::numeric_limits<std::uint64_t>::digits,
    std::uint32_t, std::uint16_t>;
  using mvbdiff_type = typename std::conditional_t<
    std::numeric_limits<difference_type>::digits==
    std::numeric_limits<std::int64_t>::digits,
    std::int32_t, std::int16_t>;

  template <class Blk>
  static void dlloc(Blk block)
    noexcept(std::is_nothrow_destructible_v<
      std::remove_pointer_t<Blk>>)
    { delete[] block; }

  struct val {
    static Tp* alloc(void)
      { return new Tp[size()]; }
    static Tp* alloc(const Tp& val) {
      auto block = new Tp[size()];
      std::fill_n(block, size(), val);
      return block;
    }
    static Tp* alloc(mvbsize_type n)
      { return new Tp[n]; }
    static Tp* alloc(mvbsize_type n, const Tp& val) {
      auto block = new Tp[n];
      std::fill_n(block, n, val);
      return block;
    }
  };

  struct index {
    static Tp** alloc(void)
      { return new Tp*[size()](); }
    static Tp** alloc(mvbsize_type n)
      { return new Tp*[n](); }
  };
  
  static consteval mvbsize_type
    size(void) noexcept
    { return static_cast<mvbsize_type>(1)<<Exp; }
  static consteval mvbsize_type
    mask(void) noexcept
    { return size()-1; }
  static constexpr mvbsize_type
    jump(mvlsize_type lvl, size_type i) noexcept
    { return static_cast<mvbsize_type>(i&mask()<<Exp*lvl)>>Exp*lvl; }
};

template <std::uint8_t Exp, class Tp>
class mv {
  private:
    using mvlsize_type = typename mvb<Exp, Tp>::mvlsize_type;
    using mvldiff_type = typename mvb<Exp, Tp>::mvldiff_type;
    using mvbsize_type = typename mvb<Exp, Tp>::mvbsize_type;
    using mvbdiff_type = typename mvb<Exp, Tp>::mvbdiff_type;

  public:
    using value_type = typename mvb<Exp, Tp>::value_type;
    using reference = typename mvb<Exp, Tp>::reference;
    using const_reference = typename mvb<Exp, Tp>::const_reference;
    using pointer = typename mvb<Exp, Tp>::pointer;
    using const_pointer = typename mvb<Exp, Tp>::const_pointer;
    using size_type = typename mvb<Exp, Tp>::size_type;
    using difference_type = typename mvb<Exp, Tp>::difference_type;

  protected:
    union mvp {
      Tp* val;
      Tp** index;
      Tp*** pindex;
    } m_root;
    size_type m_peek;
    mvlsize_type m_deep;
    mvbsize_type m_free;

    mv(void) noexcept : m_root{}, m_peek{}, m_deep{}, m_free{} {}
    // ~mv(void) noexcept(std::is_nothrow_destructible_v<Tp>) { destroy(); }

  private:
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
    void recursive_fill_blocks(mvp root,
      mvlsize_type lvl, mvbsize_type& n) {
      if( lvl==1 ) {
        // alloc block of value
        for(auto i=mvb<Exp, Tp>::jump(lvl, m_peek+mvb<Exp, Tp>::size());
            n!=0 && i<mvb<Exp, Tp>::size();
            ++i, --n, m_peek+=mvb<Exp, Tp>::size())
          root.index[i] = mvb<Exp, Tp>::val::alloc();
        return;
      }
      // alloc block of index
      for(auto i=mvb<Exp, Tp>::jump(lvl, m_peek+mvb<Exp, Tp>::size());
          n!=0 && i<mvb<Exp, Tp>::size(); ++i) {
        if( root.pindex[i]==nullptr )
          root.pindex[i] = mvb<Exp, Tp>::index::alloc();
        recursive_fill_blocks({.index=root.pindex[i]}, lvl-1, n);
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
    void recursive_fill_blocks(mvp root,
      mvlsize_type lvl, mvbsize_type& n, const Tp& val) {
      if( lvl==1 ) {
        // alloc block of value
        for(auto i=mvb<Exp, Tp>::jump(lvl, m_peek+mvb<Exp, Tp>::size());
            n!=0 && i<mvb<Exp, Tp>::size();
            ++i, --n, m_peek+=mvb<Exp, Tp>::size())
          root.index[i] = mvb<Exp, Tp>::val::alloc(val);
        return;
      }
      // alloc block of index
      for(auto i=mvb<Exp, Tp>::jump(lvl, m_peek+mvb<Exp, Tp>::size());
          n!=0 && i<mvb<Exp, Tp>::size(); ++i) {
        if( root.pindex[i]==nullptr )
          root.pindex[i] = mvb<Exp, Tp>::index::alloc();
        recursive_fill_blocks({.index=root.pindex[i]}, lvl-1, n, val);
      }
    }

////////////////////////////////////////////////////////////////////////////////

    template <class Blk>
    void recursive_backward(
      mvp root,
      Blk& head,
      Blk& prev,
      mvbsize_type n,
      mvlsize_type lvl,
      difference_type p,
      difference_type& q) {
      // traverse the blocks target
      if( lvl==1 ) {
        mvbdiff_type i=mvb<Exp, Tp>::jump(lvl, q);
        for(; p<=q && i>=0; --i, q-=mvb<Exp, Tp>::size()) {
          auto rem = mvb<Exp, Tp>::size()-n;
          auto block = reinterpret_cast<Blk>(root.pindex[i]);
          if( prev==nullptr ) {
            prev = block;
            std::move_backward(block, block+rem, block+mvb<Exp, Tp>::size());
            continue;
          }
          std::move(block+rem, block+mvb<Exp, Tp>::size(), prev);
          std::move_backward(block, block+rem, block+mvb<Exp, Tp>::size());
          prev = block;
        }
        head = reinterpret_cast<Blk>(root.pindex[i]);
        return;
      }
      // traverse block of index
      for(mvbdiff_type i=mvb<Exp, Tp>::jump(lvl, q); p<=q && i>=0; --i)
        recursive_backward<Blk>({.index=root.pindex[i]},
          head, prev, n, lvl-1, p, q);
    }

    // template <class Blk>
    // void rshift(mvlsize_type lvl, size_type i, mvbsize_type n) {
    //   Blk head, prev = nullptr;
    //   difference_type peek = m_peek>>(m_deep-lvl)*Exp;
    //   auto iblock = i&mvb<Exp, Tp>::mask();
    //   // shift the blocks with the same size
    //   if( iblock==0 ) {
    //     recursive_backward<Blk>(m_root, head, prev, n, lvl, i, peek);
    //     std::fill_n(prev, n, std::remove_pointer_t<Blk>());
    //     return;
    //   }
    //   recursive_backward<Blk>(m_root, head,
    //     prev, n, lvl, i-iblock+mvb<Exp, Tp>::size(), peek);
    //   if( iblock+n<=mvb<Exp, Tp>::size() ) {
    //     auto rem = mvb<Exp, Tp>::size()-n;
    //     if( prev!=nullptr )
    //       std::move(head+rem, head+mvb<Exp, Tp>::size(), prev);
    //     std::move_backward(head+iblock, head+rem, head+mvb<Exp, Tp>::size());
    //     std::fill_n(head+iblock, n, std::remove_pointer_t<Blk>());
    //   } else {
    //     auto rem = mvb<Exp, Tp>::size()-iblock;
    //     std::move(head+iblock, head+mvb<Exp, Tp>::size(), prev+(n-rem));
    //     std::fill_n(head+iblock, rem, std::remove_pointer_t<Blk>());
    //     std::fill_n(prev, n-rem, std::remove_pointer_t<Blk>());
    //   }
    // }

    // template <class Blk>
    // void rshift(mvlsize_type lvl,
    //   size_type i, Blk block, mvbsize_type n) {
    //   Blk head, prev = nullptr;
    //   difference_type peek = m_peek>>(m_deep-lvl)*Exp;
    //   auto iblock = i&mvb<Exp, Tp>::mask();
    //   // shift the blocks with the same size
    //   if( iblock==0 ) {
    //     recursive_backward<Blk>(m_root, head, prev, n, lvl, i, peek);
    //     std::fill_n(prev, n, std::remove_pointer_t<Blk>());
    //     return;
    //   }
    //   recursive_backward<Blk>(m_root, head,
    //     prev, n, lvl, i-iblock+mvb<Exp, Tp>::size(), peek);
    //   if( iblock+n<=mvb<Exp, Tp>::size() ) {
    //     auto rem = mvb<Exp, Tp>::size()-n;
    //     if( prev!=nullptr )
    //       std::move(head+rem, head+mvb<Exp, Tp>::size(), prev);
    //     std::move_backward(head+iblock, head+rem, head+mvb<Exp, Tp>::size());
    //     std::fill_n(head+iblock, n, std::remove_pointer_t<Blk>());
    //   } else {
    //     auto rem = mvb<Exp, Tp>::size()-iblock;
    //     std::move(head+iblock, head+mvb<Exp, Tp>::size(), prev+(n-rem));
    //     std::fill_n(head+iblock, rem, std::remove_pointer_t<Blk>());
    //     std::fill_n(prev, n-rem, std::remove_pointer_t<Blk>());
    //   }
    // }

    template <class Blk>
    void lvl_rshift(mvlsize_type lvl, size_type i, mvbsize_type n,
      std::function<void (std::remove_pointer_t<Blk>&)> create) {
      Blk head, prev = nullptr;
      difference_type peek = m_peek>>(m_deep-lvl)*Exp;
      auto iblock = i&mvb<Exp, Tp>::mask();
      // shift the blocks with the same size
      if( iblock==0 ) {
        recursive_backward<Blk>(m_root, head, prev, n, lvl, i, peek);
        std::for_each_n(prev, n, create);
        return;
      }
      recursive_backward<Blk>(m_root, head,
        prev, n, lvl, i-iblock+mvb<Exp, Tp>::size(), peek);
      if( iblock+n<=mvb<Exp, Tp>::size() ) {
        auto rem = mvb<Exp, Tp>::size()-n;
        if( prev!=nullptr )
          std::move(head+rem, head+mvb<Exp, Tp>::size(), prev);
        std::move_backward(head+iblock, head+rem, head+mvb<Exp, Tp>::size());
        std::for_each_n(head+iblock, n, create);
      } else {
        auto rem = mvb<Exp, Tp>::size()-iblock;
        std::move(head+iblock, head+mvb<Exp, Tp>::size(), prev+(n-rem));
        std::for_each_n(head+iblock, rem, create);
        std::for_each_n(prev, n-rem, create);
      }
    }

    static void recursive_build(mvp root, mvlsize_type lvl, size_type& peek) {
      if( lvl==1 ) {
        // alloc block of value
        for(auto i=mvb<Exp, Tp>::jump(lvl, peek+mvb<Exp, Tp>::size());
            i<mvb<Exp, Tp>::size(); ++i, peek+=mvb<Exp, Tp>::size())
          root.index[i] = mvb<Exp, Tp>::val::alloc();
        return;
      }
      // alloc block of index
      for(auto i=mvb<Exp, Tp>::jump(lvl, peek+mvb<Exp, Tp>::size());
          i<mvb<Exp, Tp>::size(); ++i) {
        if( root.pindex[i]==nullptr )
          root.pindex[i] = mvb<Exp, Tp>::index::alloc();
        recursive_build({.index=root.pindex[i]}, lvl-1, peek);
      }
    }
    
    static mvp build(mvlsize_type deep) {
      mvlsize_type lvl = 0;
      size_type peek = mvb<Exp, Tp>::mask();
      mvp root = {.val=mvb<Exp, Tp>::val::alloc()};
      while( deep!=0 ) {
        if( peek==end_peek(lvl) ) {
          auto block = root;
          root.index = mvb<Exp, Tp>::index::alloc();
          root.pindex[0] = block.index;
          --deep;
          ++lvl;
        }
        recursive_build(root, lvl, peek);
      }
      return root;
    }

  public:
    template <class Blk>
    void print_block(Blk block, mvbsize_type n) {
      if( block==nullptr )
        return;
      for(mvbsize_type i=0; i<n; ++i)
        std::cout<<block[i]<<" ";
      std::cout<<std::endl;
    }
    
    void rmv_testing(void) {
      // build(2);
      // print_tree(build(3), 0, 3);
      // rshift<Tp*>(2, 3, 3, [](Tp& elm) {
      //   elm = 1000;
      // });
      // rshift<Tp**>(1, 0, 2, [](auto& elm) {
      //   elm = build(0).val;
      // });

      // rshift(0, 4);
    }

    void rshift(size_type i, size_type n) {
      fill(n);
      auto block = m_root;
      for(mvlsize_type lvl=1; lvl<m_deep; ++lvl) {
        auto h = m_deep-lvl;

        std::cout<<( end_size(m_deep-lvl) )<<":"<<h<<std::endl;
        // if( lvl==m_deep )
        //   lvl_rshift<Tp*>(lvl, 0, 2, [](auto& elm) {
        //     elm = 10;
        //   });
      }
    }
////////////////////////////////////////////////////////////////////////////////

  private:
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
    bool recursive_reduce_blocks(mvp root,
      mvlsize_type lvl, mvbsize_type& n)
      noexcept(std::is_nothrow_destructible_v<Tp>) {
      if( lvl==1 ) {
        mvbdiff_type i=mvb<Exp, Tp>::jump(lvl, m_peek);
        // dealloc block of value
        for(; n!=0 && i>=0; --i, --n, m_peek-=mvb<Exp, Tp>::size()) {
          mvb<Exp, Tp>::dlloc(root.index[i]);
          root.index[i] = nullptr;
        }
        return i<0;
      }
      mvbdiff_type i=mvb<Exp, Tp>::jump(lvl, m_peek);
      // dealloc block of index
      for(; i>=0; --i) {
        if( !recursive_reduce_blocks({.index=root.pindex[i]}, lvl-1, n) )
          return false;
        mvb<Exp, Tp>::dlloc(root.pindex[i]);
        root.pindex[i] = nullptr;
      }
      return i<0;
    }

    /**
     * @brief   Destroy tree.
     * 
     * Leaving only the block of index.
     * 
     * @param   root   Root union
     * @param   lvl    Level of tree
     */
    void recursive_destroy(mvp root, mvlsize_type lvl)
      noexcept(std::is_nothrow_destructible_v<Tp>) {
      if( lvl==1 ) {
        // dealloc block of value
        for(mvbdiff_type i=mvb<Exp, Tp>::jump(lvl, m_peek);
            i>=0; --i, m_peek-=mvb<Exp, Tp>::size())
          mvb<Exp, Tp>::dlloc(root.index[i]);
        return;
      }
      // dealloc block of index
      for(mvbdiff_type i=mvb<Exp, Tp>::jump(lvl, m_peek); i>=0; --i) {
        recursive_destroy({.index=root.pindex[i]}, lvl-1);
        mvb<Exp, Tp>::dlloc(root.pindex[i]);
      }
    }

  protected:
    static constexpr size_type
      end_size(mvlsize_type deep) noexcept
      { return static_cast<size_type>(1)<<Exp*(deep+1); }
    static constexpr size_type
      end_peek(mvlsize_type deep) noexcept
      { return end_size(deep)-1; }

    void fill_blocks(mvbsize_type n) {
      if( n==0 )
        return;
      // if the tree is empty, alloc block of value as root
      if( m_peek==0 ) {
        m_root.val = mvb<Exp, Tp>::val::alloc();
        m_peek = mvb<Exp, Tp>::mask();
        --n;
      }
      while( n!=0 ) {
        // if the tree is not fit,
        // increase the tree height with alloc block of index
        if( m_peek==end_peek(m_deep) ) {
          auto block = m_root;
          m_root.index = mvb<Exp, Tp>::index::alloc();
          m_root.pindex[0] = block.index;
          ++m_deep;
        }
        // fill the tree with blocks
        recursive_fill_blocks(m_root, m_deep, n);
      }
    }

    void fill_blocks(mvbsize_type n, const Tp& val) {
      if( n==0 )
        return;
      // if the tree is empty, alloc block of value as root
      if( m_peek==0 ) {
        m_root.val = mvb<Exp, Tp>::val::alloc(val);
        m_peek = mvb<Exp, Tp>::mask();
        --n;
      }
      while( n!=0 ) {
        // if the tree is not fit,
        // increase the tree height with alloc block of index
        if( m_peek==end_peek(m_deep) ) {
          auto block = m_root;
          m_root.index = mvb<Exp, Tp>::index::alloc();
          m_root.pindex[0] = block.index;
          ++m_deep;
        }
        // fill the tree with blocks
        recursive_fill_blocks(m_root, m_deep, n, val);
      }
    }

    void reduce_blocks(mvbsize_type n)
      noexcept(std::is_nothrow_destructible_v<Tp>) {
      if( n==0 || m_peek==0 )
        return;
      // if only block of value, dealloc block of value
      if( m_deep==0 ) {
        mvb<Exp, Tp>::dlloc(m_root.val);
        m_root.val = nullptr;
        m_peek = 0;
        return;
      }
      // if the tree need to be destroyed, dealloc block of index
      if( recursive_reduce_blocks(m_root, m_deep, n) ) {
        mvb<Exp, Tp>::dlloc(m_root.index);
        m_root.index = nullptr;
        m_peek = m_deep = 0;
        return;
      }
      // if the tree doesn't need to be destroyed,
      // reduce the height with dealloc block of index
      while( m_peek<=end_peek(m_deep-1) ) {
        auto block = m_root;
        m_root.index = m_root.pindex[0];
        mvb<Exp, Tp>::dlloc(block.index);
        if( --m_deep==0 )
          return;
      }
    }
    
    Tp* push_block(void) {
      // if the tree is empty, alloc block of value as root
      if( m_peek==0 ) {
        m_root.val = mvb<Exp, Tp>::val::alloc();
        m_peek = mvb<Exp, Tp>::mask();
        return m_root.val;
      }
      auto block = m_root;
      // if the tree is not enough,
      // increase the height with alloc block of index
      if( m_peek==end_peek(m_deep) ) {
        m_root.index = mvb<Exp, Tp>::index::alloc();
        m_root.pindex[0] = block.index;
        block = m_root;
        ++m_deep;
      }
      m_peek += mvb<Exp, Tp>::size();
      // fill the block of index with alloc block of index
      for(auto lvl=m_deep; lvl>1; --lvl) {
        auto i = mvb<Exp, Tp>::jump(lvl, m_peek);
        if( block.pindex[i]==nullptr )
          block.pindex[i] = mvb<Exp, Tp>::index::alloc();
        block.index = block.pindex[i];
      }
      // alloc block of value
      return block.index[mvb<Exp, Tp>::jump(1, m_peek)]=
        mvb<Exp, Tp>::val::alloc();
    }

    Tp* head_block(void) noexcept {
      auto block = m_root;
      for(auto lvl=m_deep; lvl>0; --lvl)
        block.index = block.pindex[0];
      return block.val;
    }

    Tp* rand_block(size_type i) noexcept {
      auto block = m_root;
      for(auto lvl=m_deep; lvl>0; --lvl)
        block.index = block.pindex[mvb<Exp, Tp>::jump(lvl, i)];
      return block.val;
    }

    Tp* tail_block(void) noexcept { return access_block(m_peek); }
    void pop_block(void) noexcept(std::is_nothrow_destructible_v<Tp>)
      { return reduce_blocks(1); }

  public:
    void fill(size_type n) {
      auto new_size = size()+n;
      if( m_free>n ) {
        m_free -= n;
        return;
      }
      n -= m_free;
      mvbsize_type nblocks = (n>>Exp)+((n&mvb<Exp, Tp>::mask())!=0);
      fill_blocks(nblocks);
      m_free = capacity()-new_size;
    }

    void fill(size_type n, const Tp& val) {
      auto new_size = size()+n;
      if( m_free>n ) {
        std::fill_n(tail_block()+(mvb<Exp, Tp>::size()-m_free), n, val);
        m_free -= n;
        return;
      }
      std::fill_n(tail_block()+(mvb<Exp, Tp>::size()-m_free), m_free, val);
      n -= m_free;
      mvbsize_type nblocks = n>>Exp;
      fill_blocks(nblocks, val);
      if( (n&mvb<Exp, Tp>::mask())!=0 )
        std::fill_n(push_block(), n&mvb<Exp, Tp>::mask(), val);
      m_free = capacity()-new_size;
    }

    void reduce(size_type n)
      noexcept(std::is_nothrow_destructible_v<Tp>) {
      auto new_size = size()-n;
      if( mvb<Exp, Tp>::size()-m_free>n ) {
        m_free += n;
        return;
      }
      mvbsize_type nblocks = 1;
      n -= mvb<Exp, Tp>::size()-m_free;
      nblocks += n>>Exp;
      reduce_blocks(nblocks);
      m_free = capacity()-new_size;
    }

    void destroy(void)
      noexcept(std::is_nothrow_destructible_v<Tp>) {
      if( m_peek==0 )
        return;
      // dealloc block of value
      if( m_deep==0 ) {
        mvb<Exp, Tp>::dlloc(m_root.val);
        m_root.val = nullptr;
        m_peek = m_free = 0;
        return;
      }
      // destroy the tree
      recursive_destroy(m_root, m_deep);
      mvb<Exp, Tp>::dlloc(m_root.index);
      m_root.index = nullptr;
      m_peek = m_deep = m_free = 0;
    }

    bool empty(void) const noexcept
      { return m_peek==0; }
    size_type capacity(void) const noexcept
      { return m_peek+(m_peek!=0); }
    size_type size(void) const noexcept
      { return capacity()-m_free; }
    static consteval size_type max_size(void) noexcept
      { return static_cast<size_type>(1)<<
        (std::numeric_limits<size_type>::digits-2); }
    static consteval size_type max_exponent(void) noexcept
      { return std::numeric_limits<mvbsize_type>::digits-2; }

////////////////////////////////////////////////////////////////////////////////
  public:
    void print_tree(mvp root, mvlsize_type lvl, mvlsize_type deep) const {
      if( root.val==nullptr )
        return;
      for(size_type i=0; i<(size_type)(lvl<<1); ++i)
        std::cout<<" ";
      std::cout<<"|";
      if( lvl==deep ) {
        // iterating block of value
        for(mvbsize_type i=0; i<mvb<Exp, Tp>::size(); ++i)
          std::cout<<root.val[i]<<"|";
        std::cout<<std::endl;
        return;
      }
      // iterating block of index
      for(mvbsize_type i=0; i<mvb<Exp, Tp>::size(); ++i)
        std::cout<<(static_cast<bool>(root.pindex[i]) ? "=" : " ")<<"|";
      std::cout<<std::endl;
      // block of index call each element
      for(mvbsize_type i=0; i<mvb<Exp, Tp>::size(); ++i)
        print_tree({.index=root.pindex[i]}, lvl+1, deep);
    }
////////////////////////////////////////////////////////////////////////////////
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
    mvi(void) noexcept : m_pos{} {}
    mvi(difference_type off) noexcept : m_pos{off} {}
    bool operator==(const mvi& it) const noexcept { return m_pos==it.m_pos; }
    bool operator!=(const mvi& it) const noexcept { return m_pos!=it.m_pos; }
    bool operator<(const mvi& it) const noexcept { return m_pos<it.m_pos; }
    bool operator<=(const mvi& it) const noexcept { return m_pos<=it.m_pos; }
    bool operator>(const mvi& it) const noexcept { return m_pos>it.m_pos; }
    bool operator>=(const mvi& it) const noexcept { return m_pos>=it.m_pos; }
};

template <class V>
class rmvi : public mvi<V> {
  private:
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
    rmvi(V* vector) noexcept : m_vector{vector} {}
    rmvi(V* vector, difference_type off) noexcept :
      mvi<V>{off}, m_vector{vector} {}

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
  private:
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
    rmvci(const V* vector) noexcept : m_vector{vector} {}
    rmvci(const V* vector, difference_type off) noexcept :
      mvi<V>{off}, m_vector{vector} {}

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
  private:
    using mv<Exp, Tp>::m_root;
    using mv<Exp, Tp>::m_peek;
    using mv<Exp, Tp>::m_deep;
    using mv<Exp, Tp>::m_free;

    using mv<Exp, Tp>::end_size;
    using mv<Exp, Tp>::end_peek;
    using mv<Exp, Tp>::fill_blocks;
    using mv<Exp, Tp>::reduce_blocks;
    using mv<Exp, Tp>::push_block;
    using mv<Exp, Tp>::head_block;
    using mv<Exp, Tp>::rand_block;
    using mv<Exp, Tp>::tail_block;
    using mv<Exp, Tp>::pop_block;

    using mvlsize_type = typename mvb<Exp, Tp>::mvlsize_type;
    using mvldiff_type = typename mvb<Exp, Tp>::mvldiff_type;
    using mvbsize_type = typename mvb<Exp, Tp>::mvbsize_type;
    using mvbdiff_type = typename mvb<Exp, Tp>::mvbdiff_type;

  public:
    using mv<Exp, Tp>::fill;
    using mv<Exp, Tp>::reduce;
    using mv<Exp, Tp>::destroy;
    using mv<Exp, Tp>::empty;
    using mv<Exp, Tp>::capacity;
    using mv<Exp, Tp>::size;
    using mv<Exp, Tp>::max_size;
    using mv<Exp, Tp>::max_exponent;

    using value_type = typename mvb<Exp, Tp>::value_type;
    using reference = typename mvb<Exp, Tp>::reference;
    using const_reference = typename mvb<Exp, Tp>::const_reference;
    using pointer = typename mvb<Exp, Tp>::pointer;
    using const_pointer = typename mvb<Exp, Tp>::const_pointer;
    using size_type = typename mvb<Exp, Tp>::size_type;
    using difference_type = typename mvb<Exp, Tp>::difference_type;
    using iterator = rmvi<rpmv<Exp, Tp>>;
    using const_iterator = rmvci<rpmv<Exp, Tp>>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

////////////////////////////////////////////////////////////////////////////////
  public:
    using mv<Exp, Tp>::rmv_testing;
    void rpmv_testing(void) {
      std::cout<<"rpmv_testing()"<<std::endl;
    }
////////////////////////////////////////////////////////////////////////////////

  public:
    rpmv(void) noexcept {}
    rpmv(size_type num) : rpmv{} { fill(num); }
    rpmv(size_type num, const Tp& val) : rpmv{} { fill(num, val); }
    ~rpmv(void) noexcept(std::is_nothrow_destructible_v<Tp>) {}

    void clear(void)
      noexcept(std::is_nothrow_destructible_v<Tp>) { destroy(); }
  
  private:
    template <class Val>
    void push_elm(Val val) {
      if( m_free>0 ) {
        (*this)[m_peek-(--m_free)] = std::forward<Val>(val);
        return;
      }
      push_block()[0] = std::forward<Val>(val);
      m_free = mvb<Exp, Tp>::mask();
    }

  public:
    void push_back(const Tp& val) { push_elm(val); }
    void push_back(Tp&& val) { push_elm(std::move(val)); }
    void pop_back(void)
      noexcept(std::is_nothrow_destructible_v<Tp>) {
      if( (m_free++)!=mvb<Exp, Tp>::mask() )
        return;
      pop_block();
      m_free = 0;
    }

    reference operator[](size_type index) noexcept
      { return rand_block(index)[mvb<Exp, Tp>::jump(0, index)]; }
    const_reference operator[](size_type index) const noexcept 
      { return rand_block(index)[mvb<Exp, Tp>::jump(0, index)]; }
    reference front(void) noexcept
      { return head_block()[0]; }
    const_reference front(void) const noexcept
      { return head_block()[0]; }
    reference back(void) noexcept
      { return tail_block()[mvb<Exp, Tp>::jump(0, m_peek-m_free)]; }
    const_reference back(void) const noexcept
      { return tail_block()[mvb<Exp, Tp>::jump(0, m_peek-m_free)]; }

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

////////////////////////////////////////////////////////////////////////////////
  public:
    void print(void) const {
      std::cout<<std::endl;
      std::cout<<"deep     = "<<(size_type)m_deep<<std::endl;
      std::cout<<"free     = "<<(size_type)m_free<<std::endl;
      std::cout<<"peek     = "<<(size_type)m_peek<<std::endl;
      std::cout<<"size     = "<<(size_type)size()<<std::endl;
      std::cout<<"capacity = "<<(size_type)capacity()<<std::endl;
      std::cout<<"max      = "<<(size_type)end_peek(m_deep+1)<<std::endl;
      std::cout<<"root     = "<<m_root.val<<std::endl<<std::endl;
#ifdef RMV_DEBUG
      mv<Exp, Tp>::print_tree(m_root, 0, m_deep);
#endif
      std::cout<<std::endl;
    }
////////////////////////////////////////////////////////////////////////////////
};

}

#endif /* RSFR_RMV_H */
