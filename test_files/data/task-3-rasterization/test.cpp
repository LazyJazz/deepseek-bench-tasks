#include "raster.h"
#include "test_support.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <vector>

bool Same(Pixel a,Pixel b){return a.r==b.r&&a.g==b.g&&a.b==b.b&&a.a==b.a;}
using Points=std::set<std::pair<int,int>>;
Points Colored(const Image&i,Pixel color){Points p;for(int y=0;y<i.height();++y)for(int x=0;x<i.width();++x)if(Same(i(x,y),color))p.insert({x,y});return p;}
Points LineOracle(int x0,int y0,int x1,int y1){Points p;std::int64_t dx=std::abs(std::int64_t(x1)-x0),sx=x0<x1?1:-1,dy=-std::abs(std::int64_t(y1)-y0),sy=y0<y1?1:-1,err=dx+dy;for(;;){p.insert({x0,y0});if(x0==x1&&y0==y1)break;auto e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}return p;}
struct ReferenceImage{std::uint32_t width{},height{};std::vector<Pixel> pixels;};
ReferenceImage ReadReference(const char*name){std::ifstream f(std::string(FIXTURE_DIR)+"/"+name,std::ios::binary);CHECK(f.is_open());ReferenceImage r;CHECK(bool(f.read(reinterpret_cast<char*>(&r.width),4)));CHECK(bool(f.read(reinterpret_cast<char*>(&r.height),4)));r.pixels.resize(static_cast<std::size_t>(r.width)*r.height);CHECK(bool(f.read(reinterpret_cast<char*>(r.pixels.data()),static_cast<std::streamsize>(r.pixels.size()*sizeof(Pixel)))));return r;}
bool SameRgb(Pixel a,Pixel b){return a.r==b.r&&a.g==b.g&&a.b==b.b;}
void ValidateReference(const Image&i,const char*name){auto r=ReadReference(name);CHECK(i.width()==static_cast<int>(r.width)&&i.height()==static_cast<int>(r.height));for(int y=0;y<i.height();++y)for(int x=0;x<i.width();++x){bool found=false;auto expected=r.pixels[static_cast<std::size_t>(y)*r.width+x];for(int oy=-1;oy<=1&&!found;++oy)for(int ox=-1;ox<=1&&!found;++ox){int px=x+ox,py=y+oy;if(px>=0&&px<i.width()&&py>=0&&py<i.height())found=SameRgb(expected,i(px,py));}CHECK(found);}}
void CheckReferenceLines(){int dx[]={-80,-80,-80,-20,0,20,80,80,80,80,80,20,0,-20,-80,-80},dy[]={0,20,80,80,80,80,80,20,0,-20,-80,-80,-80,-80,-80,-20};Image i(201,201,{0,0,0,0});for(int n=0;n<16;++n)DrawLine(i,100,100,100+dx[n],100+dy[n],{255,255,255,255});ValidateReference(i,"lines.rgba");}
void CheckReferenceTriangles(){int dx[]={-80,-80,-80,-20,0,20,80,80,80,80,80,20,0,-20,-80,-80},dy[]={0,20,80,80,80,80,80,20,0,-20,-80,-80,-80,-80,-80,-20};Image i(201,201,{0,0,0,0});for(int n=0;n<16;n+=2){int j=(n+1)&15;FillTriangle(i,100,100,100+dx[n],100+dy[n],100+dx[j],100+dy[j],{255,255,255,255});}ValidateReference(i,"triangles.rgba");}
void CheckReferenceCircles(){Image i(201,201,{0,0,0,0});FillCircle(i,100,100,80,{127,127,127,255});FillCircle(i,50,50,50,{255,0,255,255});FillCircle(i,50,150,50,{255,255,0,255});FillCircle(i,150,50,50,{0,255,255,255});FillCircle(i,150,150,50,{255,255,255,255});ValidateReference(i,"circles.rgba");}
void CheckReferenceRectangles(){Image i(201,201,{0,0,0,0});std::ifstream f(std::string(FIXTURE_DIR)+"/rectangle.data",std::ios::binary);CHECK(f.is_open());for(int n=0;n<10;++n){int v[4];Pixel p;CHECK(bool(f.read(reinterpret_cast<char*>(v),sizeof(v))));CHECK(bool(f.read(reinterpret_cast<char*>(&p),sizeof(p))));FillRectangle(i,v[0],v[1],v[2],v[3],p);}ValidateReference(i,"rectangles.rgba");}
int main(int argc,char**argv){return Run([&]{CHECK(argc==2);std::string g=argv[1];Pixel white{255,255,255,255},red{255,0,0,255};
  if(g=="pixel"){
    Image i(3,2,white);DrawPixel(i,1,1,red);CHECK(Same(i(1,1),red));for(auto p:std::vector<std::pair<int,int>>{{-1,0},{3,0},{0,-1},{0,2}})DrawPixel(i,p.first,p.second,red);CHECK(Colored(i,red)==Points{{1,1}});Image empty(0,0);DrawPixel(empty,0,0,red);
  }else if(g=="line"){CheckReferenceLines();
    for(auto e:std::vector<std::array<int,4>>{{1,1,8,3},{8,3,1,1},{2,8,5,0},{5,0,2,8},{-8,4,15,4},{-20,-20,-3,-1},{4,4,4,4}}){Image i(10,10,white);DrawLine(i,e[0],e[1],e[2],e[3],red);Points expected;for(auto p:LineOracle(e[0],e[1],e[2],e[3]))if(p.first>=0&&p.first<10&&p.second>=0&&p.second<10)expected.insert(p);CHECK(Colored(i,red)==expected);}
  }else if(g=="rectangle"){CheckReferenceRectangles();
    Image i(6,5,white);FillRectangle(i,4,3,1,-2,red);Points expected;for(int y=0;y<=3;++y)for(int x=1;x<=4;++x)expected.insert({x,y});CHECK(Colored(i,red)==expected);FillRectangle(i,20,20,30,30,red);CHECK(Colored(i,red)==expected);
  }else if(g=="circle"){CheckReferenceCircles();
    for(auto c:std::vector<std::array<int,3>>{{4,3,3},{0,0,4},{5,5,0},{2,2,-1}}){Image i(9,8,white);FillCircle(i,c[0],c[1],c[2],red);Points expected;if(c[2]>=0)for(int y=0;y<8;++y)for(int x=0;x<9;++x){std::int64_t dx=x-c[0],dy=y-c[1];if(dx*dx+dy*dy<=std::int64_t(c[2])*c[2])expected.insert({x,y});}CHECK(Colored(i,red)==expected);}
  }else if(g=="triangle"){CheckReferenceTriangles();
    auto check=[&](std::array<int,6>v){Image i(11,10,white);FillTriangle(i,v[0],v[1],v[2],v[3],v[4],v[5],red);Points expected;auto cross=[](std::int64_t ax,std::int64_t ay,std::int64_t bx,std::int64_t by,std::int64_t px,std::int64_t py){return (bx-ax)*(py-ay)-(by-ay)*(px-ax);};auto area=cross(v[0],v[1],v[2],v[3],v[4],v[5]);for(int y=0;y<10;++y)for(int x=0;x<11;++x){if(area==0){if(LineOracle(v[0],v[1],v[2],v[3]).count({x,y})||LineOracle(v[2],v[3],v[4],v[5]).count({x,y})||LineOracle(v[4],v[5],v[0],v[1]).count({x,y}))expected.insert({x,y});}else{auto a=cross(v[0],v[1],v[2],v[3],x,y),b=cross(v[2],v[3],v[4],v[5],x,y),c=cross(v[4],v[5],v[0],v[1],x,y);if((a>=0&&b>=0&&c>=0)||(a<=0&&b<=0&&c<=0))expected.insert({x,y});}}CHECK(Colored(i,red)==expected);};
    check({1,1,9,2,3,8});check({3,8,9,2,1,1});check({-5,2,5,-4,14,8});check({1,1,5,3,9,5});
  }else CHECK(false);
});}
