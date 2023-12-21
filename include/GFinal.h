/*
 *  Copyright 2020 Mike Reed
 */

#ifndef GFinal_DEFINED
#define GFinal_DEFINED

#include "GCanvas.h"
#include "GPath.h"
#include "GShader.h"
#include <array>

/* A 4x5 matrix to transform unpremul GColors.
 *
 *  new_color = [0 4  8 12 16] [orig_color.r]
 *              [1 5  9 13 17] [orig_color.g]
 *              [2 6 10 14 18] [orig_color.b]
 *              [3 7 11 15 19] [orig_color.a]
 *
 *  e.g. new.r = M[0] * old.r + M[4] * old.g + M[8] * old.b + M[12] * old.a + M[16]
 *       new.g = M[1] * old.r + ...
 *       ...
 *
 *  The identity matrix would look like this
 *      [1 0 0 0 0]
 *      [0 1 0 0 0]
 *      [0 0 1 0 0]
 *      [0 0 0 1 0]
 */
struct GColorMatrix {
    std::array<float, 20> fMat;

    GColorMatrix() : fMat{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0} {}
    GColorMatrix(const std::array<float, 20> src) : fMat{src} {}

    float operator[](unsigned i) const {
        assert(i < 20);
        return fMat[i];
    }

    float& operator[](unsigned i) {
        assert(i < 20);
        return fMat[i];
    }
};

/**
 * Override and implement these methods. You must implement GCreateFinal() to return your subclass.
 *
 * Each method has a default (do nothing) impl, so you only need to override the ones you intended to implement.
 */
class GFinal {
public:
    virtual ~GFinal() {}

    /**
     *  Return a radial-gradient shader.
     *
     *  This is a shader defined by a circle with center and a radius.
     *  The array of colors are evenly distributed between the center (color[0]) out to
     *  the radius (color[count-1]). Beyond the radius, it respects the TileMode.
     */
    virtual std::unique_ptr<GShader> createRadialGradient(GPoint center, float radius,
                                                          const GColor colors[], int count,
                                                          GShader::TileMode mode) {
        return nullptr;
    }

    /*
     *  Returns an instance to a shader that will proxy to a "realShader", and transform
     *  its output using the GColorMatrix provided.
     *
     *  Note: the GColorMatrix is defined to operate on unpremul GColors
     */
    virtual std::unique_ptr<GShader> createColorMatrixShader(const GColorMatrix&,
                                                             GShader* realShader) {
        return nullptr;
    }

    /**
     *  Construct a path that, when drawn, will look like a stroke of the specified polygon.
     *  - count is the number of points in the polygon (it will be >= 2)
     *  - width is the thickness of the stroke that should be centered on the polygon edges
     *  - isClosed specifies if the polygon should appear closed (true) or open (false).
     *
     *  Any caps or joins needed should be round (circular).
     */
    virtual GPath strokePolygon(const GPoint[], int count, float width, bool isClosed) {
        return GPath();
    }

    /*
     *  Draw the corresponding mesh constructed from a quad with each side defined by a
     *  quadratic bezier, evaluating them to produce "level" interior lines (same convention
     *  as drawQuad().
     *
     *  pts[0]    pts[1]    pts[2]
     *  pts[7]              pts[3]
     *  pts[6]    pts[5]    pts[4]
     *
     *  Evaluate values within the mesh using the Coons Patch formulation:
     *
     *  value(u,v) = TB(u,v) + LR(u,v) - Corners(u,v)
     *
     *     Top is quadratic bezier: pts[0], pts[1], pts[2]
     *  Bottom is quadratic bezier: pts[6], pts[5], pts[4]
     *    Left is quadratic bezier: pts[0], pts[7], pts[6]
     *   Right is quadratic bezier: pts[2], pts[3], pts[4]
     *
     *  Where
     *      TB is computed by first evaluating the Top and Bottom curves at (u), and then
     *      linearly interpolating those points by (v)
     *
     *      LR is computed by first evaluating the Left and Right curves at (v), and then
     *      linearly interpolating those points by (u)
     *
     *      Corners is computed by our standard "drawQuad" evaluation using the 4 corners 0,2,4,6
     */
    virtual void drawQuadraticCoons(GCanvas*, const GPoint pts[8], const GPoint tex[4],
                                    int level, const GPaint&) {}
};

/**
 *  Implement this to return ain instance of your subclass of GFinal.
 */
std::unique_ptr<GFinal> GCreateFinal();

#endif
