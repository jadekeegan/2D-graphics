/*
 *  Copyright 2023 Jade Keegan
 */

#ifndef edges_DEFINED
#define edges_DEFINED

#include "include/GCanvas.h"
#include "include/GRect.h"
#include "include/GColor.h"
#include "include/GBitmap.h"
#include "path.h"
#include "helpers.h"
#include <deque>

struct Edge {
  // m = (x1-x0)/(y1-y0)
  float m, b;
  int top, bottom;
  float currX;
  int wind;
};

Edge makeEdge(GPoint p0, GPoint p1, int wind) {
    if (p0.y > p1.y) {
      std::swap(p0, p1);
    }

    int top = myRound(p0.y);
    int bottom = myRound(p1.y);

    if (top == bottom) {
      // return an invalid edge
      return {-1, -1, -1, -1, -1, 0 };
    }

    const float m = (p1.x - p0.x) / (p1.y - p0.y);
    const float b = p0.x - (p0.y * m);

    return { .m=m, .b=b, .top=top, .bottom=bottom, .currX = m * ((float) top + 0.5f) + b, .wind=(wind ? 1 : -1) };
}

// clip edge
std::vector<Edge> clipEdges(int bottom, int right, GPoint p0, GPoint p1) {
  std::vector<Edge> edges;

  bool wind;

  // make p0 = top pt, p1 = bottom pt
  if (p0.y == p1.y) {
    return edges;
  }

  if (p0.y > p1.y) {
    std::swap(p0, p1);
    wind = false;
  } else {
    wind = true;
  }

  // if vertically above/below canvas
  if (p1.y <= 0 || p0.y >= bottom) {
    return edges;
  }

  const float m = (p1.x - p0.x) / (p1.y - p0.y);
  const float b = p0.x - (p0.y * m);

  // clip top
  if (p0.y < 0) {
      p0.x = b;
      p0.y = 0;
  }

  // clip bottom
  if (p1.y > bottom) {
    p1.x = m * bottom + b;
    p1.y = bottom;
  }

  // clip sides
  if (p0.x > p1.x) {
    std::swap(p0, p1);
  }

  // left project
  if (p1.x <= 0) { // not on canvas, left project
    Edge edge = makeEdge({0, p0.y}, {0, p1.y}, wind);

    if (edge.top != -1) {
      edges.push_back(edge);
    }

    return edges;
  }

  // right project
  if (p0.x >= right) { // not on canvas, right project
    Edge edge = makeEdge({right, p0.y}, {right, p1.y}, wind);
    
    if (edge.top != -1) {
      edges.push_back(edge);
    }

    return edges;
  }
  
  // left straddle
  if (p0.x < 0) { // left point straddling
    float newY = (b / m) * -1;

    Edge edge = makeEdge({0, p0.y}, {0, newY}, wind);

    p0 = {0, newY};

    if (edge.top != -1) {
      edges.push_back(edge);
    }
  }
  
  // right straddle
  if (p1.x > right) {
    float newY = (right-b) / m;

    Edge edge = makeEdge({right, p1.y}, {right, newY}, wind);

    p1 = {(float) right, newY};

    if (edge.top != -1) {
      edges.push_back(edge);
    }
  }

  Edge edge = makeEdge(p0, p1, wind);
  if (edge.top != -1) {
    edges.push_back(edge);
  }

  return edges;
}

std::vector<Edge> buildEdges(GBitmap device, int count, const GPoint points[]) {
  // clip points
  int dBottom = device.height();
  int dRight = device.width();

  std::vector<Edge> edges;

  // build edges
  for (int i=0; i<count; i++) {
    GPoint p0 = points[i];
    GPoint p1 = points[(i+1) % count];

    std::vector<Edge> new_edges = clipEdges(dBottom, dRight, p0, p1);

    if (new_edges.size() > 0) {
      edges.insert(edges.end(), new_edges.begin(), new_edges.end());
    }
  }
  return edges;
}

int numQuadSegments(GPoint pts[3]) {
  GPoint E = (pts[0] - 2 * pts[1] + pts[2]) * 0.25f; // (A - 2B + C) / 4
  float magnitude = sqrt(E.x * E.x + E.y * E.y);

  // (int)ceil(sqrt(|E|/tolerance))
  return (int) ceil(sqrt(magnitude * 4)); // tolerance = 0.25
}

int numCubicSegments(GPoint pts[4]) {
  GPoint E0 = pts[0] - 2 * pts[1] + pts[2];         // A - 2B + C
  GPoint E1 = pts[1] - 2 * pts[2] + pts[3];         // B - 2C + D

  float Ex = std::max(abs(E0.x), abs(E1.x)); 
  float Ey = std::max(abs(E0.y), abs(E1.y));
  
  float magnitude = sqrt(Ex * Ex + Ey * Ey);

  // (int)ceil(sqrt((3*|E|)/(4*tolerance)));
  return (int) ceil(sqrt(magnitude * 3)); // tolerance = 0.25
}

std::vector<Edge> buildPathEdges(GPath path, int width, int height) {
  std::vector<Edge> edges;
  GPoint pts[GPath::kMaxNextPoints];
  GPath::Edger edger(path);
  GPath::Verb v;

  while ((v = edger.next(pts)) != GPath::kDone) {
      std::vector<Edge> new_edges;
      int num_segments;
      GPoint p0, p1;
      float t;

      switch (v) {
          case GPath::kLine:
              new_edges = clipEdges(height, width, pts[0], pts[1]);

              if (new_edges.size() > 0) {
                  edges.insert(edges.end(), new_edges.begin(), new_edges.end());
              }
                              
              break;

          case GPath::kQuad:
            // optimize to use chopQuad where f'(t)y = 0 (derivative of f(t) where change in y = 0)
            num_segments = numQuadSegments(pts);
            p0 = pts[0];

            for (int i=1; i < num_segments; i++) {
              t = i * (1.f/num_segments);

              p1 = { getQuadCurvePoint(pts[0].x, pts[1].x, pts[2].x, t).ABC, 
                     getQuadCurvePoint(pts[0].y, pts[1].y, pts[2].y, t).ABC };

              new_edges = clipEdges(height, width, p0, p1);

              if (new_edges.size() > 0) {
                  edges.insert(edges.end(), new_edges.begin(), new_edges.end());
              }

              p0 = p1;
            } 

            new_edges = clipEdges(height, width, p0, pts[2]);
            if (new_edges.size() > 0) {
              edges.insert(edges.end(), new_edges.begin(), new_edges.end());
            }
            
            break;

          case GPath::kCubic:
            num_segments = numCubicSegments(pts);
            p0 = pts[0];

            for (int i=1; i < num_segments; i++) {
              t = i * (1.f/num_segments);

              p1 = { getCubicCurvePoint(pts[0].x, pts[1].x, pts[2].x, pts[3].x, t).ABCD, 
                     getCubicCurvePoint(pts[0].y, pts[1].y, pts[2].y, pts[3].y, t).ABCD };

              new_edges = clipEdges(height, width, p0, p1);

              if (new_edges.size() > 0) {
                  edges.insert(edges.end(), new_edges.begin(), new_edges.end());
              }

              p0 = p1;
            } 

            new_edges = clipEdges(height, width, p0, pts[3]);
            if (new_edges.size() > 0) {
              edges.insert(edges.end(), new_edges.begin(), new_edges.end());
            }

            break;
      }
  }
  return edges;
}

bool isValidEdge(Edge e, int y) {
  return e.top <= y && e.bottom > y;
}

// check if e0 > e1
bool compareEdges(Edge e0, Edge e1) {
  return (e0.top < e1.top) ? true:
          (e1.top < e0.top) ? false:
          (e0.currX < e1.currX) ? true:
          (e1.currX < e0.currX) ? false:
          (e0.m < e1.m);
}

bool compareEdgesX(Edge e0, Edge e1) {
  return e0.currX < e1.currX;
}

#endif