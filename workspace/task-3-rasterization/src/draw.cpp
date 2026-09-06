#include "raster.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

void DrawPixel(Image &image,int x,int y,Pixel color){if(x>=0&&x<image.width()&&y>=0&&y<image.height())image(x,y)=color;}
void DrawLine(Image &image,int x0,int y0,int x1,int y1,Pixel color){
  std::int64_t dx=std::llabs(std::int64_t(x1)-x0),sx=x0<x1?1:-1;
  std::int64_t dy=-std::llabs(std::int64_t(y1)-y0),sy=y0<y1?1:-1,err=dx+dy;
  for(;;){DrawPixel(image,x0,y0,color);if(x0==x1&&y0==y1)break;auto twice=2*err;if(twice>=dy){err+=dy;x0+=int(sx);}if(twice<=dx){err+=dx;y0+=int(sy);}}
}
void FillRectangle(Image &image,int x0,int y0,int x1,int y1,Pixel color){
  if(x0>x1)std::swap(x0,x1);if(y0>y1)std::swap(y0,y1);x0=std::max(x0,0);y0=std::max(y0,0);x1=std::min(x1,image.width()-1);y1=std::min(y1,image.height()-1);
  for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)image(x,y)=color;
}
void FillCircle(Image &image,int cx,int cy,int radius,Pixel color){
  if(radius<0)return;const std::int64_t squared=std::int64_t(radius)*radius;
  const int x0=std::max(-radius,cx>0?-cx:-radius),x1=std::min(radius,image.width()-1-cx);
  const int y0=std::max(-radius,cy>0?-cy:-radius),y1=std::min(radius,image.height()-1-cy);
  for(int dy=y0;dy<=y1;++dy)for(int dx=x0;dx<=x1;++dx)if(std::int64_t(dx)*dx+std::int64_t(dy)*dy<=squared)DrawPixel(image,cx+dx,cy+dy,color);
}
void FillTriangle(Image &image,int x0,int y0,int x1,int y1,int x2,int y2,Pixel color){
  auto edge=[](std::int64_t ax,std::int64_t ay,std::int64_t bx,std::int64_t by,std::int64_t px,std::int64_t py){return (bx-ax)*(py-ay)-(by-ay)*(px-ax);};
  const auto area=edge(x0,y0,x1,y1,x2,y2);if(area==0){DrawLine(image,x0,y0,x1,y1,color);DrawLine(image,x1,y1,x2,y2,color);DrawLine(image,x2,y2,x0,y0,color);return;}
  const int min_x=std::max(0,std::min({x0,x1,x2})),max_x=std::min(image.width()-1,std::max({x0,x1,x2}));
  const int min_y=std::max(0,std::min({y0,y1,y2})),max_y=std::min(image.height()-1,std::max({y0,y1,y2}));
  for(int y=min_y;y<=max_y;++y)for(int x=min_x;x<=max_x;++x){auto a=edge(x0,y0,x1,y1,x,y),b=edge(x1,y1,x2,y2,x,y),c=edge(x2,y2,x0,y0,x,y);if((a>=0&&b>=0&&c>=0)||(a<=0&&b<=0&&c<=0))image(x,y)=color;}
}
