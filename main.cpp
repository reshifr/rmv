#define RMV_DEBUG

#include <cstdio>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include <rmv.hpp>
#include <algorithm>

#if defined(__GNUG__) && defined(_OPENMP)
# include <parallel/algorithm>
#elif defined(_MSC_VER)
# include <execution>
#endif

using namespace std;
using namespace chrono;

class timer {
  private:
    mt19937_64 rng;
    decltype(high_resolution_clock::now()) x, y;

  public:
    timer(void) :
      rng(high_resolution_clock::now().time_since_epoch().count()) {}
    void start(void) {
      x = high_resolution_clock::now(); }

    template <class P>
    int64_t result(void) {
      y = high_resolution_clock::now();
      return duration_cast<P>(y-x).count();
    }

    template <class T>
    T random(size_t l, size_t h) {
      static_assert(is_scalar<T>::value, "Error: Use scalar type!\n");
      uniform_int_distribution<T> rand(l, h);
      return rand(rng);
    }
};

#define MILLION(_n) (_n ## 000 ## 000 ## ULL)
#define M MILLION(100)
#define N 19

int main(void) {
  timer clock;

  rsfr::rmv<1, int> mv;
  mv.extend(N);

  for(auto& elm : mv)
    elm = clock.random<int>(1, 9);

  clock.start();
#if defined(__GNUG__) && defined(_OPENMP)
  __gnu_parallel::sort(mv.begin(), mv.end());
#elif defined(_MSC_VER)
  sort(execution::par, mv.begin(), mv.end());
#endif
  cout<<"clock = "<<clock.result<milliseconds>()<<" ms"<<endl;

  mv.debug();
}
