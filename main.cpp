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
    void start(void) { x = high_resolution_clock::now(); }
    
    template <class P>
    int64_t result(void) {
      y = high_resolution_clock::now();
      return duration_cast<P>(y-x).count();
    }

    template <class T>
    T random(T l, T h) {
      static_assert(is_scalar<T>::value, "Error: Use scalar type!\n");
      uniform_int_distribution<T> rand(l, h);
      return rand(rng);
    }
};

#define MILLION(_n) (_n##000##000##ULL)
#define M MILLION(1)
#define N 333
#define E 3

int main(void) {
  timer clock;
  rsfr::rcmv<E, int> mv(N);
  for(auto& elm : mv)
    elm = clock.random(1, 99);

  int x = 300;
  mv.push_back(x);
  mv.push_back(333);

  clock.start();
#if defined(__GNUG__) && defined(_OPENMP)
  __gnu_parallel::stable_sort(mv.rbegin(), mv.rend());
#elif defined(_MSC_VER)
  stable_sort(execution::par, mv.rbegin(), mv.rend());
#endif
  cout<<"clock = "<<clock.result<milliseconds>()<<" ms"<<endl<<endl;

  for(auto& elm : mv)
    cout<<elm<<" ";
  cout<<endl<<endl;
  mv.debug();
}
