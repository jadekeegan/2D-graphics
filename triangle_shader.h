/*
 *  Copyright 2023 Jade Keegan
 */

#ifndef triangle_shader_DEFINED
#define triangle_shader_DEFINED

#include "include/GShader.h"
#include "include/GMatrix.h"
#include "include/GBitmap.h"

class TriangleShader : public GShader {
  public:
    TriangleShader(const GPoint pts[3], const GColor colors[])
      : fColors(colors, colors+3) {

      // solve and find u,v
      GPoint U = pts[1] - pts[0];
      GPoint V = pts[2] - pts[0];

      fUnitMatrix = { U.x, V.x, pts[0].x,
                      U.y, V.y, pts[0].y };

      fColorDiff1 = colors[1] - colors[0];
      fColorDiff2 = colors[2] - colors[0];
    }

    bool isOpaque() override {
      return fColors[0].a == 1.0 && fColors[1].a == 1.0 && fColors[2].a == 1.0;
    }

    bool setContext(const GMatrix& ctm) override {
      return (ctm * fUnitMatrix).invert(&fInverseCTM);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
      GPoint point = { x+0.5f, y+0.5f };
      GPoint p = fInverseCTM * point;

      GColor color = p.x * fColorDiff1 + p.y * fColorDiff2 + fColors[0];
      GColor colorInc = fInverseCTM[0] * fColorDiff1 + fInverseCTM[3] * fColorDiff2;
    
      for (int i = 0; i < count; ++i) {
    
        row[i] = colorToPixel(color);
        
        p.x += fInverseCTM[0];
        color += colorInc;
      }
    }

    private:
      std::vector<GColor> fColors;
      GMatrix fInverseCTM, fUnitMatrix;
      GColor fColorDiff1, fColorDiff2;
      int fNumColors;

      // Color to Pixel
      GPixel colorToPixel(GColor color) {
          return GPixel_PackARGB((int) myRound(color.a * 255.f), 
                                  (int) myRound(color.r * color.a * 255.f), 
                                  (int) myRound(color.g * color.a * 255.f), 
                                  (int) myRound(color.b * color.a * 255.f));
      }
};

std::unique_ptr<GShader> GCreateTriangleShader(const GPoint pts[3], const GColor colors[]) {
  return std::unique_ptr<GShader>(new TriangleShader(pts, colors));
}

#endif