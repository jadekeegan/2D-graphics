/*
 *  Copyright 2023 Jade Keegan
 */

#include "include/GMatrix.h"
#include "include/GPath.h"
#include "path.h"
#include <iostream>
using namespace std;

#define dT_Constant 0.5519150244935105707435627f;

enum Direction {
    kCW_Direction,  // clockwise
    kCCW_Direction, // counter-clockwise
};

void GPath::addRect(const GRect& rect, Direction direction) {
  this->moveTo({rect.left, rect.top});

  if (direction == Direction::kCW_Direction) {
    this->lineTo({rect.right, rect.top});
    this->lineTo({rect.right, rect.bottom});
    this->lineTo({rect.left, rect.bottom});
  } else {
    this->lineTo({rect.left, rect.bottom});
    this->lineTo({rect.right, rect.bottom});
    this->lineTo({rect.right, rect.top});
  }
}

void GPath::addPolygon(const GPoint pts[], int count) {
  assert(count >= 2);

  this->moveTo(pts[0]);
  for (int i=1; i < count; ++i) {
    this->lineTo(pts[i]);
  }
}

GRect GPath::bounds() const {  
  int count = this->fPts.size();

  if (count == 0) {
    return GRect::LTRB(0,0,0,0);
  }  

  if (count == 1) {
    return GRect::XYWH(fPts[0].x, fPts[0].y, 0, 0);
  }

  float minX = fPts[0].x;
  float maxX = fPts[0].x;
  float minY = fPts[0].y;
  float maxY = fPts[0].y;

  GPoint pts[GPath::kMaxNextPoints];
  GPath::Edger iter(*this);
  GPath::Verb v;
  while ((v = iter.next(pts)) != GPath::kDone) {
      GPoint A, B, C, D;
      float t1, t2;
      float ax, bx, cx;
      float ay, by, cy;

      float px0 = minX;
      float py0 = minY;
      float px1 = minX; 
      float py1 = minY;

      switch (v) {
        case GPath::kLine:
          minX = min({ minX, pts[0].x, pts[1].x });
          maxX = max({ maxX, pts[0].x, pts[1].x } );
          minY = min({ minY, pts[0].y, pts[1].y });
          maxY = max({ maxY, pts[0].y, pts[1].y });

          break;

        case GPath::kQuad:
          A = pts[0];
          B = pts[1];
          C = pts[2];

          // extrema value
          t1 = (A.x - B.x) / (A.x - 2 * B.x + C.x);
          t2 = (A.y - B.y) / (A.y - 2 * B.y + C.y);

          // test extrema for x
          if(t1 >= 0 && t1 <= 1) {
            px0 = getQuadCurvePoint(A.x, B.x, C.x, t1).ABC;
            py0 = getQuadCurvePoint(A.y, B.y, C.y, t1).ABC;
          }

          //test extrema for y
          if(t2 >= 0 && t2 <= 1) {
            px1 = getQuadCurvePoint(A.x, B.x, C.x, t2).ABC;
            py1 = getQuadCurvePoint(A.y, B.y, C.y, t2).ABC;
          }

          minX = min({ minX, px0, px1, A.x, C.x });
          maxX = max({ maxX, px0, px1, A.x, C.x });
          minY = min({ minY, py0, py1, A.y, C.y });
          maxY = max({ maxY, py0, py1, A.y, C.y });

          break;

        case GPath::kCubic:
          A = pts[0];
          B = pts[1];
          C = pts[2];
          D = pts[3];

          ax = -A.x + 3 * B.x - 3 * C.x + D.x;
          bx = 2 * A.x - 4 * B.x + 2 * C.x;
          cx = -A.x + B.x;

          ay = -A.y + 3 * B.y - 3 * C.y + D.y;
          by = 2 * A.y - 4 * B.y + 2 * C.y;
          cy = -A.y + B.y;

          if (ax == 0) {
            t1 = -cx / bx;
            t2 = -1;
          } else {
            // f'(t)x == 0
            t1 = (-bx + sqrt(bx*bx - 4 * ax * cx)) / (2 * ax);
            t2 = (-bx - sqrt(bx*bx - 4 * ax * cx)) / (2 * ax);
          }
          
          if(t1 >= 0 && t1 <= 1) {
            px0 = getCubicCurvePoint(A.x, B.x, C.x, D.x, t1).ABCD;
            py0 = getCubicCurvePoint(A.y, B.y, C.y, D.y, t1).ABCD;
          }

          if(t2 >= 0 && t2 <= 1) {
            px1 = getCubicCurvePoint(A.x, B.x, C.x, D.x, t2).ABCD;
            py1 = getCubicCurvePoint(A.y, B.y, C.y, D.y, t2).ABCD;
          }

          minX = min({ minX, px0, px1, A.x, D.x });
          maxX = max({ maxX, px0, px1, A.x, D.x });
          minY = min({ minY, py0, py1, A.y, D.y });
          maxY = max({ maxY, py0, py1, A.y, D.y });

          if (ay == 0) {
            t1 = -cy / by;
            t2 = -1;
          } else {
            // f'(t)y == 0
            t1 = (-by + sqrt(by*by - 4 * ay * cy)) / (2 * ay);
            t2 = (-by - sqrt(by*by - 4 * ay * cy)) / (2 * ay);
          }

          if(t1 >= 0 && t1 <= 1) {
            px0 = getCubicCurvePoint(A.x, B.x, C.x, D.x, t1).ABCD;
            py0 = getCubicCurvePoint(A.y, B.y, C.y, D.y, t1).ABCD;
          }

          if(t2 >= 0 && t2 <= 1) {
            px1 = getCubicCurvePoint(A.x, B.x, C.x, D.x, t2).ABCD;
            py1 = getCubicCurvePoint(A.y, B.y, C.y, D.y, t2).ABCD;
          }

          minX = min({ minX, px0, px1, A.x, D.x });
          maxX = max({ maxX, px0, px1, A.x, D.x });
          minY = min({ minY, py0, py1, A.y, D.y });
          maxY = max({ maxY, py0, py1, A.y, D.y });

          break;
      }
  }
  return GRect{minX,minY,maxX,maxY};
}

void GPath::addCircle(GPoint center, float radius, Direction direction) {
  float x = center.x;
  float y = center.y;

  float distance = radius * dT_Constant;

  this->moveTo(x + radius, y);

  if (direction == kCW_Direction) {
    this->cubicTo({ x + radius, y + distance }, { x + distance, y + radius }, { x, y + radius }); // ConPt1, ConPt2, Bottom
    this->cubicTo({ x - distance, y + radius }, { x - radius, y + distance }, { x - radius, y }); // ConPt3, ConPt4, Left
    this->cubicTo({ x - radius, y - distance }, { x - distance, y - radius }, { x, y - radius});  // ConPt5, ConPt6, Top
    this->cubicTo({ x + distance, y - radius }, { x + radius, y - distance }, { x + radius, y }); // ConPt7, ConPt8, Right
  } else {
    this->cubicTo({ x + radius, y - distance }, { x + distance, y - radius }, { x, y - radius});
    this->cubicTo({ x - distance, y - radius }, { x - radius, y - distance }, { x - radius, y });
    this->cubicTo({ x - radius, y + distance }, { x - distance, y + radius }, { x, y + radius });
    this->cubicTo({ x + distance, y + radius }, { x + radius, y + distance }, { x + radius, y });
  }
}

void GPath::ChopQuadAt(const GPoint src[3], GPoint dst[5], float t) {
  GPoint A = src[0];
  GPoint B = src[1];
  GPoint C = src[2];

  QuadCurve ABCx = getQuadCurvePoint(A.x, B.x, C.x, t);
  QuadCurve ABCy = getQuadCurvePoint(A.y, B.y, C.y, t);

  dst[0] = A;
  dst[1] = { ABCx.AB, ABCy.AB };
  dst[2] = { ABCx.ABC, ABCy.ABC };
  dst[3] = { ABCx.BC, ABCy.BC };
  dst[4] = C;
}

void GPath::ChopCubicAt(const GPoint src[4], GPoint dst[7], float t) {
  GPoint A = src[0];
  GPoint B = src[1];
  GPoint C = src[2];
  GPoint D = src[3];

  CubicCurve ABCDx = getCubicCurvePoint(A.x, B.x, C.x, D.x, t);
  CubicCurve ABCDy = getCubicCurvePoint(A.y, B.y, C.y, D.y, t);

  dst[0] = A;
  dst[1] = { ABCDx.ABC.AB, ABCDy.ABC.AB };
  dst[2] = { ABCDx.ABC.ABC, ABCDy.ABC.ABC };
  dst[3] = { ABCDx.ABCD, ABCDy.ABCD };
  dst[4] = { ABCDx.BCD.ABC, ABCDy.BCD.ABC };
  dst[5] = { ABCDx.BCD.BC, ABCDy.BCD.BC }; // wrong change later
  dst[6] = D;
}

void GPath::transform(const GMatrix& matrix) {
  matrix.mapPoints(this->fPts.data(), this->fPts.data(), this->fPts.size());
}

QuadCurve getQuadCurvePoint(float A, float B, float C, float t) {
  float AB = A + t * (B - A);
  float BC = B + t * (C - B);
  float ABC = AB + t * (BC - AB);

  return { AB, BC, ABC };
}

CubicCurve getCubicCurvePoint(float A, float B, float C, float D, float t) {
  QuadCurve ABC = getQuadCurvePoint(A, B, C, t);
  QuadCurve BCD = getQuadCurvePoint(B, C, D, t);
  float ABCD = ABC.ABC + t * (BCD.ABC - ABC.ABC);

  return { ABC, BCD, ABCD };
}