#include <iostream>
using namespace std;

#include "include/GShader.h"
#include "include/GMatrix.h"
#include "include/GBitmap.h"
#include "include/GPoint.h"

class RadialGradientShader : public GShader {

public:
    RadialGradientShader() {}

    RadialGradientShader(GPoint center, float radius, const GColor colors[], int count, GShader::TileMode mode)
      : fColors(colors, colors + count) {
        fCenter = center;
        fRadius = radius;
        fNumColors = count;
        fMode = mode;

        fUnitMatrix = { 1, 0, center.x,
                        0, 1, center.y };
    }

    bool isOpaque() {
        return false;
    }

    bool setContext(const GMatrix& ctm) {
        return (ctm * fUnitMatrix).invert(&fInverseCTM);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        GPoint p = fInverseCTM * GPoint{x + 0.5f, y + 0.5f};
        GPoint c = fInverseCTM * fCenter;
        float dx = fInverseCTM[0];

        for (int i = 0; i < count; i++) {
            if (fNumColors == 1) {
                row[i] = colorToPixel(fColors[0]);
                continue;
            }

            float t = sqrtf(pow(p.x, 2) + pow(p.y, 2)) / fRadius;
  
            if (fMode == GShader::TileMode::kClamp) {
                t = GPinToUnit(t);

            } else if (fMode == GShader::TileMode::kRepeat) {
                t -= floor(t);

            } else if (fMode == GShader::TileMode::kMirror) {
                if ((int) floor(t) % 2 == 0) {
                    t -= floor(t);
                } else {
                    t = 1.f - (t - floor(t));
                }
            }

            int idx = floor((float)(fNumColors - 1) * t);
            float position = 1.f / (float)(fNumColors - 1);
            float j = idx * position;

            t = GPinToUnit((t - j) / position);
            row[i] = colorToPixel(fColors[idx] * (1.f-t) + fColors[idx+1 >= fNumColors ? idx : idx+1] * t);

            p.x += dx;
        }
    }

    private:
      GPoint fCenter;
      float fRadius;
      std::vector<GColor> fColors;
      GMatrix fUnitMatrix;
      GMatrix fInverseCTM;
      int fNumColors;
      GShader::TileMode fMode;

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
};