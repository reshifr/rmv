#define RMV_DEBUG

#include "rmv.h"
#include <cstdio>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
// #include <parallel/algorithm>

class timer {
  public:
    using nano = std::chrono::nanoseconds;
    using micro = std::chrono::microseconds;
    using mili = std::chrono::milliseconds;

  private:
    decltype(std::chrono::high_resolution_clock::now()) x, y;

  public:
    void start(void) {
      timer::x = std::chrono::high_resolution_clock::now();
    }

    template <class Pre>
    std::int64_t result(void) {
      timer::y = std::chrono::high_resolution_clock::now();
      return std::chrono::duration_cast<Pre>(timer::y-timer::x).count();
    }
};

#define MILLION(_n) (_n ## 000 ## 000 ## ULL)

#ifdef RMV_DEBUG
# define N 14
#else
# define N MILLION(1)
#endif

int main(void) {
  rsfr::rmv<1, int> mv;

  mv.push_block();

#ifdef RMV_DEBUG
  std::cout<<mv;
#endif
}
