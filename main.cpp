#define RMV_DEBUG

#include <rmv.hpp>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include <algorithm>

#if defined(__GNUG__) && defined(_OPENMP)
# include <parallel/algorithm>
#elif defined(_MSC_VER)
# include <execution>
#endif

namespace rmvmain {

using namespace std;
using namespace chrono;

class clock {
  private:
    decltype(high_resolution_clock::now()) a, b;
  
  public:
    mt19937_64 rng;
    random_device dev;

  public:
    clock(void) { rng = mt19937_64(dev()); }
    void start(void) { a = high_resolution_clock::now(); }
    
    template<class P>
    decltype(auto) result(void) {
      b = high_resolution_clock::now();
      return duration_cast<P>(b-a).count();
    }

    template<class T>
    T random(T lo, T hi) {
      uniform_int_distribution<T> rd(lo, hi);
      return rd(rng);
    }
};

#define Mil(_n) (_n##000##000)
#define N Mil(1)
#define E 4

void run(void) {
  clock cl;
  rsfr::rpmv<E, int> mv(10);
  for(auto& elm : mv)
    elm = cl.random(1, 99);

  int x = 300;
  mv.push_back(x);
  mv.push_back(333);

  cl.start();
#if defined(__GNUG__) && defined(_OPENMP)
  __gnu_parallel::stable_sort(mv.rbegin(), mv.rend());
#elif defined(_MSC_VER)
  stable_sort(execution::par, mv.rbegin(), mv.rend());
#endif
  cout<<"clock = "<<cl.result<milliseconds>()<<" ms"<<endl<<endl;
  mv.print();

  for(auto& elm : mv)
    cout<<elm<<" ";
  cout<<endl<<endl;
}

}

int main(void) {
  rmvmain::run();
  return EXIT_SUCCESS;
}
