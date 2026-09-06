#include "raytracer.h"
#include "test_support.h"

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

bool Near(float a,float b,float e=1e-4f){return std::abs(a-b)<=e;}
bool Near(glm::vec3 a,glm::vec3 b,float e=1e-4f){return Near(a.x,b.x,e)&&Near(a.y,b.y,e)&&Near(a.z,b.z,e);}
Material Diff(glm::vec3 c={1,1,1}){return {c,MaterialType::kLambertian};}
Material Mirror(glm::vec3 c={1,1,1}){return {c,MaterialType::kSpecular};}
Scene ReferenceScene(){Scene s;const float ri=.8f;s.spheres={{{0,0,0},1e3,Diff({.6,.7,.8})},{{0,0,0},3000,Diff({.6,.7,.8})},{{0,2,0},2,Diff({.2001,.5,.18})},{{2,1.6,3},1.6,Diff({ri,.5*ri,ri})},{{-4,1,3},1,Mirror({ri,ri,ri})},{{3.5,1.4,-2.5},1.4,Mirror({ri,ri,ri})},{{-1.5,1.2,-4.5},1.2,Diff({.5*ri,ri,ri})},{{-5.5,3,-1.5},3,Mirror({ri,ri,ri})},{{-3.5,2.5,-12.5},2.5,Diff({ri,ri,.5*ri})}};s.triangles={{{-100,0,-100},{100,0,-100},{-100,0,100},Diff({.8,.8,.8})},{{100,0,100},{100,0,-100},{-100,0,100},Diff({.8,.8,.8})}};auto light=glm::normalize(glm::vec3{0,6,15})*900.0f;s.lights={{{light},{7e5,7e5,7e5}}};s.ambient={.3,.3,.3};return s;}
std::vector<std::uint8_t> ReadReference(std::uint32_t&w,std::uint32_t&h){std::ifstream f(std::string(FIXTURE_DIR)+"/raytracing.rgba",std::ios::binary);CHECK(f.is_open());CHECK(bool(f.read(reinterpret_cast<char*>(&w),4)));CHECK(bool(f.read(reinterpret_cast<char*>(&h),4)));std::vector<std::uint8_t> p(static_cast<std::size_t>(w)*h*4);CHECK(bool(f.read(reinterpret_cast<char*>(p.data()),static_cast<std::streamsize>(p.size()))));return p;}
void CheckReferenceScene(){std::uint32_t w,h;auto expected=ReadReference(w,h);CHECK(w==1280&&h==720);auto s=ReferenceScene();glm::vec3 origin{10,5,10},forward=glm::normalize(glm::vec3{0,1,0}-origin),right=glm::normalize(glm::cross(glm::vec3{0,1,0},forward)),up=glm::normalize(glm::cross(forward,right));float scale=std::tan(40.0*std::acos(-1.0)/360.0),aspect=double(w)/h;std::vector<std::uint8_t> actual(expected.size(),255);for(std::uint32_t y=0;y<h;++y)for(std::uint32_t x=0;x<w;++x){glm::vec3 color{};for(int sy=0;sy<4;++sy)for(int sx=0;sx<4;++sx){float nx=(2*(x+(sx+.5)/4.0)/w-1)*aspect*scale,ny=(2*(y+(sy+.5)/4.0)/h-1)*scale;color=color+SampleRay(s,{origin,glm::normalize(forward+right*nx-up*ny)},8);}color=color/16.0f;auto index=(static_cast<std::size_t>(y)*w+x)*4;actual[index]=static_cast<std::uint8_t>(std::lround(std::clamp(color.x,0.0f,1.0f)*255));actual[index+1]=static_cast<std::uint8_t>(std::lround(std::clamp(color.y,0.0f,1.0f)*255));actual[index+2]=static_cast<std::uint8_t>(std::lround(std::clamp(color.z,0.0f,1.0f)*255));}std::size_t matches=0;for(std::uint32_t y=0;y<h;++y)for(std::uint32_t x=0;x<w;++x){bool found=false;for(int oy=-1;oy<=1&&!found;++oy)for(int ox=-1;ox<=1&&!found;++ox){int px=static_cast<int>(x)+ox,py=static_cast<int>(y)+oy;if(px<0||py<0||px>=static_cast<int>(w)||py>=static_cast<int>(h))continue;bool close=true;auto e=(static_cast<std::size_t>(y)*w+x)*4,a=(static_cast<std::size_t>(py)*w+px)*4;for(int c=0;c<3;++c)if(std::abs(int(expected[e+c])-int(actual[a+c]))>4)close=false;found=close;}matches+=found;}CHECK(double(matches)/(w*h)>.99);}
int main(int argc,char**argv){return Run([&]{CHECK(argc==2);std::string g=argv[1];
  if(g=="sphere"){
    Sphere s{{0,0,0},1,Diff()};HitRecord h;CHECK(IntersectSphere({{0,0,-3},{0,0,1}},s,.001,100,h));CHECK(Near(h.t,2)&&Near(h.point,{0,0,-1})&&Near(h.normal,{0,0,-1}));
    CHECK(IntersectSphere({{0,0,0},{2,0,0}},s,.001,100,h));CHECK(Near(h.t,1));CHECK(!IntersectSphere({{0,2,-3},{0,0,1}},s,.001,100,h));CHECK(!IntersectSphere({{0,0,-3},{0,0,1}},s,2.1,3.9,h));
  }else if(g=="triangle"){
    Triangle t{{-1,-1,0},{1,-1,0},{0,1,0},Diff()};HitRecord h;CHECK(IntersectTriangle({{0,0,2},{0,0,-1}},t,.001,100,h));CHECK(Near(h.t,2)&&Near(h.normal,{0,0,1}));
    Triangle reverse{t.v2,t.v1,t.v0,t.material};CHECK(IntersectTriangle({{0,0,2},{0,0,-1}},reverse,.001,100,h));CHECK(Near(h.normal,{0,0,1}));CHECK(!IntersectTriangle({{2,2,2},{0,0,-1}},t,.001,100,h));CHECK(!IntersectTriangle({{0,0,2},{1,0,0}},t,.001,100,h));
  }else if(g=="closest"){
    Scene s;s.spheres={{{0,0,3},1,Diff({1,0,0})},{{0,0,6},2,Diff({0,1,0})}};s.triangles={{{-2,-2,1},{2,-2,1},{0,2,1},Diff({0,0,1})}};HitRecord h;CHECK(CastRay(s,{{0,0,0},{0,0,1}},.001,100,h));CHECK(Near(h.t,1)&&Near(h.material.albedo,{0,0,1}));CHECK(!CastRay(s,{{0,0,0},{1,0,0}},.001,100,h));
  }else if(g=="lighting"){
    Scene s;s.ambient={.1,.2,.3};s.lights={{{0,0,10},{100,200,300}}};HitRecord h{1,{0,0,0},{0,0,1},Diff({.5,.25,1})};CHECK(Near(CalculateLighting(s,h),{.55,.55,3.3}));
    s.spheres.push_back({{0,0,5},1,Diff()});CHECK(Near(CalculateLighting(s,h),{.05,.05,.3}));
  }else if(g=="reflection"){
    Scene s;s.ambient={.2,.4,.6};s.triangles.push_back({{-10,-10,0},{10,-10,0},{0,10,0},Mirror({.5,.5,.5})});
    CHECK(Near(SampleRay(s,{{0,0,2},{0,0,-1}},4),{.1,.2,.3}));CHECK(Near(SampleRay(s,{{0,0,2},{0,0,-1}},0),{0,0,0}));
    s.spheres.push_back({{0,0,4},1,Diff({.4,.5,.6})});CHECK(Near(SampleRay(s,{{0,0,2},{0,0,-1}},4),{.04,.1,.18}));
    Scene miss;miss.ambient={.3,.2,.1};CHECK(Near(SampleRay(miss,{{0,0,0},{9,0,0}},8),miss.ambient));
  }else if(g=="scene"){CheckReferenceScene();
  }else CHECK(false);
});}
