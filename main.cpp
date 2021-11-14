#define RMV_DEBUG_

#include "rmv.h"
#include <cstdio>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <parallel/algorithm>

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
# define N 32
#else
# define N MILLION(1)
#endif

int main(void) {
  timer clock;

  // std::cout<<"RMV        = "<<sizeof(rsfr::rmv<1, int>)<<std::endl;
  // std::cout<<"RMV_CACHED = "<<sizeof(rsfr::rmv<1, int, true>)<<std::endl;
  // std::cout<<"VECTOR     = "<<sizeof(std::vector<int>)<<std::endl;

  rsfr::rmv<1, unsigned __int64> vector(N);

  std::mt19937_64 rng(3232);
  std::uniform_int_distribution<int> rand_elm(1, 99);
  for(std::size_t i=0; i<N; ++i)
    vector[i] = rand_elm(rng);

  clock.start();
  __gnu_parallel::sort(vector.begin(), vector.end());
  std::cout<<"clock = "<<clock.result<timer::mili>()<<"ms"<<std::endl;

  // for(auto elm : vector) {
  //   std::cout<<elm<<" ";
  // }

  // clock.start();
  // for(std::size_t i=0; i<N; ++i)
  //   vector[i] = i;
  // std::cout<<"clock = "<<clock.result<timer::mili>()<<"ms"<<std::endl;

  // std::vector<std::size_t> real_vector(N);
  // clock.start();
  // for(std::size_t i=0; i<N; ++i)
  //   real_vector[i] = i;
  // std::cout<<"clock = "<<clock.result<timer::mili>()<<"ms"<<std::endl;

#ifdef RMV_DEBUG
  std::cout<<vector;
#endif
}
