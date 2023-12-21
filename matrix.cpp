/*
 *  Copyright 2023 Jade Keegan
 */

#include "include/GMatrix.h"

GMatrix::GMatrix() {
  fMat[0] = 1.f;  fMat[1] = 0;    fMat[2] = 0;
  fMat[3] = 0;    fMat[4] = 1.f;  fMat[5] = 0;
}

GMatrix GMatrix::Translate(float tx, float ty) {
  return GMatrix(1, 0, tx,
                 0, 1, ty);
}

GMatrix GMatrix::Scale(float sx, float sy) {
  return GMatrix(sx, 0, 0,
                 0, sy, 0);
}

GMatrix GMatrix::Rotate(float radians) {
  return GMatrix(cos(radians), -sin(radians), 0,
                 sin(radians), cos(radians), 0);
}

GMatrix GMatrix::Concat(const GMatrix& a, const GMatrix& b) {
  return GMatrix((a[0]*b[0]+a[1]*b[3]), (a[0]*b[1]+a[1]*b[4]), (a[0]*b[2]+a[1]*b[5]+a[2]),
                 (a[3]*b[0]+a[4]*b[3]), (a[3]*b[1]+a[4]*b[4]), (a[3]*b[2]+a[4]*b[5]+a[5]));
}

bool GMatrix::invert(GMatrix* inverse) const {
  float a = this->fMat[0];
  float b = this->fMat[1];
  float c = this->fMat[2];
  float d = this->fMat[3];
  float e = this->fMat[4];
  float f = this->fMat[5];

  // a * (e*i - f*h) - b * (d*i - f*g) + c * (d*h - e*g) simplifies to..
  float determinant = a * e - b * d;
  
  if (determinant == 0) {
    return false;
  }

  float constant = 1.f / determinant;

  inverse->fMat[0] = e * constant;
  inverse->fMat[1] = -b * constant;
  inverse->fMat[2] = (b * f - c * e) * constant;
  inverse->fMat[3] = -d * constant;
  inverse->fMat[4] = a * constant;
  inverse->fMat[5] = (c * d - a * f) * constant;

  return true;
}

void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const {
  for (int i=0; i<count; ++i) {
    GPoint p = src[i];

    float x = this->fMat[0] * p.x + this->fMat[1] * p.y + this->fMat[2];
    float y = this->fMat[3] * p.x + this->fMat[4] * p.y + this->fMat[5];

    dst[i] = {x, y};
  }
}