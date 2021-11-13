#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define NUM(_exponent) (1<<(_exponent))
#define DELTA(_exponent) ((NUM(_exponent)-1))
#define SHIFT(_exponent, _lvl) ((_lvl)*(_exponent))
#define END(_exponent, _lvl) (((uint64_t)1<<SHIFT(_exponent, (_lvl)+1))-1)
#define MASK(_exponent, _lvl) (DELTA(_exponent)<<SHIFT(_exponent, _lvl))
#define INDEX(_exponent, _index, _lvl) \
  (((_index)&MASK(_exponent, _lvl))>>SHIFT(_exponent, _lvl))

typedef int rmvval;

struct rmv {
  uint8_t height;
  uint8_t exponent;
  // uint32_t exponent;
  uint64_t tag;
  rmvval* block;
  void** root;
  uint64_t end;
};
  
struct rmv* rmv_init(uint8_t exponent) {
  struct rmv* vec = calloc(1, sizeof(struct rmv));
  vec->exponent = exponent;
  vec->root = malloc(sizeof(rmvval)*NUM(exponent));
  vec->block = (rmvval*)vec->root;
  vec->end = DELTA(exponent);
  return vec;
}

void rmv_extend(struct rmv* vec) {
  void** node = vec->root;
  if( vec->end==END(vec->exponent, vec->height) ) {
    vec->root = calloc(NUM(vec->exponent), sizeof(void*));
    vec->root[0] = node;
    vec->height++;
  }
  vec->end += NUM(vec->exponent);
  node = vec->root;
  for(int h=vec->height; h>1; --h) {
    int index = INDEX(vec->exponent, vec->end, h);
    if( node[index]==NULL )
      node[index] = calloc(NUM(vec->exponent), sizeof(void*));
    node = node[index];
  }
  node[INDEX(vec->exponent, vec->end, 1)] =
    malloc(sizeof(rmvval)*NUM(vec->exponent));
}

void rmv_insert(struct rmv* vec, uint64_t index, rmvval val) {
  if( vec->tag==(index>>vec->exponent) ) {
    vec->block[INDEX(vec->exponent, index, 0)] = val;
    return;
  }
  void** node = vec->root;
  for(int h=vec->height; h>0; --h) {
    node = node[INDEX(vec->exponent, index, h)];
  }
  ((rmvval*)node)[INDEX(vec->exponent, index, 0)] = val;
  vec->tag = index>>vec->exponent;
  vec->block = (rmvval*)node;
}

#define MILI (CLOCKS_PER_SEC/1000)
#define MILLION(_n) (_n ## 000 ## 000 ## ULL)

int main(void) {
  struct rmv* vec = rmv_init(8);
  uint64_t n = MILLION(1);
  while( (vec->end+1)<n )
    rmv_extend(vec);

  clock_t cl = clock();
  for(uint64_t i=0; i<n; ++i)
    rmv_insert(vec, i, i+1);
  printf("Time = %ld miliseconds\n", (clock()-cl)/MILI);
  printf("Space = %" PRIu64 "\n", vec->end+1);
  printf("Struct = %" PRIu64 " bytes\n", sizeof(struct rmv));
  return 0;
}
