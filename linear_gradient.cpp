/*
 *  Copyright 2023 Jade Keegan
 */

#include "include/GShader.h"
#include "include/GMatrix.h"
#include "include/GBitmap.h"

class LinearGradientShader : public GShader {
  public:
    LinearGradientShader(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode mode)
      : fColors(colors, colors+count),
        fNumColors(count), 
        fTileMode(mode) {
        
      // create colorDiff vector
      for (int i=0; i<count-1; ++i) {
        fColorDiff.push_back(colors[i+1]-colors[i]);
      }

      float dx = p1.x - p0.x;
      float dy = p1.y - p0.y;

      fUnitMatrix = { dx, -dy, p0.x, dy, dx, p0.y };
    }

    bool isOpaque() override {
      for (int i = 0; i < fNumColors; i++) {
        if (fColors[i].a != 1.0) {
          return false;
        }
      }
		  return true;
    }

    bool setContext(const GMatrix& ctm) override {
      return (ctm * fUnitMatrix).invert(&fInverseCTM);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
      GPoint point = { x+0.5f, y+0.5f };
      GPoint p = fInverseCTM * point; 

      if (fNumColors == 1) {
        for (int i = 0; i < count; ++i) {
          row[i] = colorToPixel(fColors[0]);
        }
      } else {
        for (int i = 0; i < count; ++i) {
          float x;

          switch (fTileMode) {
            case kMirror:
              x = mirror(p.x);
              break;

            case kRepeat:
              x = repeat(p.x);
              break;

            case kClamp:
              x = clamp(p.x);
              break;
          }
          
          int j = GFloorToInt(x);
          float t = x - j;

          GColor c;
          if (t == 0) {
            c = fColors[j];
          } else {
            c = fColors[j] + (t * fColorDiff[j]);
          }
          
          row[i] = colorToPixel(c);
          
          p.x += fInverseCTM[0];
        }
      }
    }

    private:
      std::vector<GColor> fColorDiff;
      std::vector<GColor> fColors;
      GMatrix fInverseCTM;
      GMatrix fUnitMatrix;
      GShader::TileMode fTileMode;
      int fNumColors;

      // Color to Pixel
      GPixel colorToPixel(GColor color) {
          return GPixel_PackARGB((int) myRound(color.a * 255.f), 
                                  (int) myRound(color.r * color.a * 255.f), 
                                  (int) myRound(color.g * color.a * 255.f), 
                                  (int) myRound(color.b * color.a * 255.f));
      }
      
      int myRound(float x) {
        if (x < 0 ) {
            return 0;
        }

        return (int) floor(x + 0.5f);
      }   

      float repeat(float point) {
        return (point - GFloorToInt(point)) * (fNumColors-1);
      }

      float mirror(float point) {
        int p = GFloorToInt(abs(point));

        if ((p % 2) == 0) {
          return (point - GFloorToInt(point)) * (fNumColors-1);
        } else {
          return (GCeilToInt(point) - point) * (fNumColors-1);
        }
      }

      float clamp(float point) {
        return GPinToUnit(point) * (fNumColors-1);
      }
};

std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode mode) {
  if (count < 1) {
    return nullptr;
  }

  return std::unique_ptr<GShader>(new LinearGradientShader(p0, p1, colors, count, mode));
}