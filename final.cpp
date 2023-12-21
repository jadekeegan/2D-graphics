#include "include/GFinal.h"
#include "include/GShader.h"
#include "radial_gradient.h"

class MyFinal : public GFinal {
public:
    MyFinal() {}

    std::unique_ptr<GShader> createRadialGradient(GPoint center, float radius,
                                                    const GColor colors[], int count,
                                                    GShader::TileMode mode) override {
        return  std::unique_ptr<GShader>(new RadialGradientShader(center, radius, colors, count, mode));
    }

    GPath addLine(GPath path, GPoint p0, GPoint p1, float width) {
      if (p0.x > p1.x) { std::swap(p0, p1); }

      float dx = p1.x - p0.x;
      float dy = p1.y - p0.y;

      float distance = width/2.;

      GPoint u = {-dy,dx};
      u = u * (1./sqrt(u.x * u.x + u.y * u.y));

      path.moveTo(p0 + u * distance);
      path.lineTo(p0 - u * distance);
      path.lineTo(p1 - u * distance);
      path.lineTo(p1 + u * distance);
      path.addCircle(p0, distance);
      path.addCircle(p1, distance);

      return path;
    }

    GPath strokePolygon(const GPoint* points, int count, float width, bool isClosed) {
      GPath path;

      for (int i=0; i<count-1; i++) {
        GPoint p0 = points[i];
        GPoint p1 = points[i+1];

        path = addLine(path, p0, p1, width);
      }

      if (isClosed) {
        path = addLine(path, points[0], points[count-1], width);
      }

      return path;
    }
};

std::unique_ptr<GFinal> GCreateFinal() {
    return std::unique_ptr<GFinal>(new MyFinal());
}