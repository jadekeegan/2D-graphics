/*
 *  Copyright 2023 Jade Keegan
 */

#ifndef combined_shader_DEFINED
#define combined_shader_DEFINED

#include "include/GShader.h"
#include "include/GMatrix.h"
#include "include/GBitmap.h"

class CombinedShader : public GShader {
  public:
      CombinedShader(GShader* shader0, GShader* shader1)
          : fShader0(shader0), fShader1(shader1) {}

      bool isOpaque() override { return fShader0->isOpaque() && fShader1->isOpaque(); }

      bool setContext(const GMatrix& ctm) override {
          return fShader0->setContext(ctm) && fShader1->setContext(ctm);
      }
      
      void shadeRow(int x, int y, int count, GPixel row[]) override {
          GPixel row0[count];
          GPixel row1[count];

          fShader0->shadeRow(x, y, count, row0);
          fShader1->shadeRow(x, y, count, row1);

          for (int i=0; i<count; i++) {
            row[i] = multiplyPixels(row0[i], row1[i]);
          }
      }
      
  private:
      GShader* fShader0;
      GShader* fShader1;

      GPixel multiplyPixels(GPixel pixel0, GPixel pixel1) {
        
        int a = GRoundToInt((GPixel_GetA(pixel0) * GPixel_GetA(pixel1)) / 255.0f);
        int r = GRoundToInt((GPixel_GetR(pixel0) * GPixel_GetR(pixel1)) / 255.0f);
        int g = GRoundToInt((GPixel_GetG(pixel0) * GPixel_GetG(pixel1)) / 255.0f);
        int b = GRoundToInt((GPixel_GetB(pixel0) * GPixel_GetB(pixel1)) / 255.0f);
        
        return GPixel_PackARGB(a, r, g, b);
      }
  };

#endif