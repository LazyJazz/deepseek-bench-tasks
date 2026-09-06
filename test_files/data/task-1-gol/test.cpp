#include "game_of_life_lib.h"
#include "test_support.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <new>
#include <vector>

namespace allocation_watch {
bool active = false;
std::size_t calls = 0;

void *Allocate(std::size_t size) {
  if (active) ++calls;
  if (void *memory = std::malloc(size == 0 ? 1 : size)) return memory;
  throw std::bad_alloc();
}

struct Scope {
  Scope() { calls = 0; active = true; }
  ~Scope() { active = false; }
};
}  // namespace allocation_watch

void *operator new(std::size_t size) { return allocation_watch::Allocate(size); }
void *operator new[](std::size_t size) { return allocation_watch::Allocate(size); }
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept { std::free(memory); }

using Grid = std::vector<uint8_t>;
void CheckStep(int w, int h, const Grid &before, const Grid &expected) {
  CHECK(before.size() == static_cast<std::size_t>(w*h));
  Grid guarded(before.size()+2, 173);
  std::copy(before.begin(), before.end(), guarded.begin()+1);
  update_step(w,h,guarded.data()+1);
  CHECK(guarded.front() == 173 && guarded.back() == 173);
  for (std::size_t i=0;i<expected.size();++i) CHECK(bool(guarded[i+1]) == bool(expected[i]));
}
int main(int argc,char **argv) { return Run([&] {
  CHECK(argc==2); const std::string g=argv[1];
  if(g=="patterns") {
    Grid blinker={0,0,0,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,0,0,0};
    Grid horizontal={0,0,0,0,0, 0,0,0,0,0, 0,1,1,1,0, 0,0,0,0,0, 0,0,0,0,0};
    CheckStep(5,5,blinker,horizontal); CheckStep(5,5,horizontal,blinker);
    Grid block={0,0,0,0, 0,1,1,0, 0,1,1,0, 0,0,0,0}; CheckStep(4,4,block,block);
    Grid glider(64), shifted(64);
    for(auto p: {std::pair<int,int>{2,1},{3,2},{1,3},{2,3},{3,3}}) {
      glider[p.second*8+p.first]=1; shifted[(p.second+1)*8+p.first+1]=1;
    }
    for(int i=0;i<4;++i) update_step(8,8,glider.data());
    for(int i=0;i<64;++i) CHECK(bool(glider[i])==bool(shifted[i]));
  } else if(g=="edges") {
    update_step(0,0,nullptr); update_step(0,5,nullptr); update_step(7,0,nullptr);
    CheckStep(1,1,{1},{0}); CheckStep(1,1,{0},{0});
    CheckStep(1,3,{1,1,1},{0,1,0}); CheckStep(3,1,{1,1,1},{0,1,0});
    CheckStep(2,2,{1,1,1,0},{1,1,1,1});
    CheckStep(4,3,{1,0,0,1, 0,0,0,0, 1,0,0,0},Grid(12,0));
  } else if(g=="nonbinary") {
    CheckStep(2,2,{2,128,255,0},{1,1,1,1});
    CheckStep(3,1,{128,2,255},{0,1,0});
  } else if(g=="exhaustive") {
    // Exhaust all 512 neighborhoods; neighbor counts by bit positions, no grid oracle.
    for(unsigned mask=0;mask<512;++mask) {
      Grid a(11,231); unsigned neighbors=0;
      for(int i=0;i<9;++i) { a[i+1]=(mask>>i)&1; if(i!=4) neighbors+=a[i+1]; }
      bool center=neighbors==3 || (neighbors==2 && ((mask>>4)&1));
      update_step(3,3,a.data()+1); CHECK(bool(a[5])==center);
      CHECK(a.front()==231 && a.back()==231);
    }
  } else if(g=="regression") {
    for(auto shape: {std::pair<int,int>{10,10},{15,25},{25,15},{40,40}}) {
      int w=shape.first,h=shape.second; auto file=Data(std::to_string(w)+"-"+std::to_string(h)+"-100-5.data",true);
      Grid actual(w*h),expected(w*h);
      for(int round=0;round<5;++round) {
        CHECK(bool(file.read(reinterpret_cast<char*>(actual.data()),actual.size())));
        for(int step=0;step<100;++step) {
          CHECK(bool(file.read(reinterpret_cast<char*>(expected.data()),expected.size())));
          CheckStep(w,h,actual,expected); actual=expected;
        }
      }
      CHECK(file.peek()==std::char_traits<char>::eof());
    }
  } else if(g=="large") {
    // Many separated oscillators; expected states are known without a solver.
    constexpr int w=512,h=384; Grid vertical(w*h),horizontal(w*h);
    for(int y=3;y<h-3;y+=8) for(int x=3;x<w-3;x+=8) {
      for(int d=-1;d<=1;++d) {vertical[(y+d)*w+x]=1; horizontal[y*w+x+d]=1;}
    }
    Grid actual=vertical;
    for(int step=0;step<40;++step) {
      update_step(w,h,actual.data()); const auto &expected=step%2?vertical:horizontal;
      for(int i=0;i<w*h;++i) CHECK(bool(actual[i])==bool(expected[i]));
    }
  } else if(g=="allocation") {
    Grid buffer(257*193);
    for(std::size_t i=0;i<buffer.size();++i) buffer[i]=(i*37+i/11)%5==0?255:0;
    {
      allocation_watch::Scope scope;
      update_step(257,193,buffer.data());
    }
    CHECK(allocation_watch::calls==0);
  } else CHECK(false);
}); }
