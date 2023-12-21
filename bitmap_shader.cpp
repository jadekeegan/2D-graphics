/*
 * Copyright 2023 Jade Keegan
 */

#include "include/GShader.h"
#include "include/GMatrix.h"
#include "include/GBitmap.h"

class BitmapShader : public GShader {
  public:
    BitmapShader(const GBitmap& bitmap, const GMatrix& localInverse, GShader::TileMode mode)
      : fDevice(bitmap), 
        fLocalInverse(localInverse),
        fTileMode(mode) {}

    bool isOpaque() override {
      return fDevice.isOpaque();
    }

    bool setContext(const GMatrix& ctm) override {
      GMatrix inverseCTM;
      if (!ctm.invert(&inverseCTM)) {
        return false;
      }

      fInverseCTM = GMatrix::Concat(fLocalInverse, inverseCTM);

      return true;
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {  
      GPoint p = fInverseCTM * GPoint({ x+0.5f, y+0.5f });
      
      for (int i = 0; i < count; ++i) {
        int newX, newY;
        switch (fTileMode) {
          case kMirror:
            newX = clamp(mirror(p.x, fDevice.width()), fDevice.width());
            newY = clamp(mirror(p.y, fDevice.height()), fDevice.height());
            break;

          case kRepeat:
            newX = repeat(p.x, fDevice.width());
            newY = repeat(p.y, fDevice.height());
            break;

          case kClamp:
            newX = clamp(p.x, fDevice.width());
            newY = clamp(p.y, fDevice.height());
            break;
        }
        row[i] = *fDevice.getAddr(newX, newY);

        p.x += fInverseCTM[0]; // this is A
        p.y += fInverseCTM[3]; // this is D
      }
    }

  private:
    GBitmap fDevice;
    GMatrix fInverseCTM;
    GMatrix fLocalInverse;
    GShader::TileMode fTileMode;

    int repeat(float point, int dimension) {
      float pinned = point / dimension;

      return (pinned - GFloorToInt(pinned)) * dimension;
    }

    int mirror(float point, int dimension) {
      float pinned = point / dimension;

      int p = GFloorToInt(abs(pinned));

      if ((p % 2) == 0) {
        return (pinned - GFloorToInt(pinned)) * dimension;
      } else {
        return (GCeilToInt(pinned) - pinned) * dimension;
      }
    }

    int clamp(float x, int bound) {
      return GFloorToInt(x < 0 ? 0: x > bound-1 ? bound-1: x);
    }
};

std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bitmap, const GMatrix& localInverse, GShader::TileMode mode) {
  if (!bitmap.pixels()) {
    return nullptr;
  }

  return std::unique_ptr<GShader>(new BitmapShader(bitmap, localInverse, mode));
}