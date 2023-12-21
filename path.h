/*
 *  Copyright 2023 Jade Keegan
 */

#ifndef path_DEFINED
#define path_DEFINED

struct QuadCurve {
  float AB;
  float BC;
  float ABC;
};

struct CubicCurve {
  QuadCurve ABC;
  QuadCurve BCD;
  float ABCD;
};


QuadCurve getQuadCurvePoint(float A, float B, float C, float t);

CubicCurve getCubicCurvePoint(float A, float B, float C, float D, float t);

#endif /* path_DEFINED */