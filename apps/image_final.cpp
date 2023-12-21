/**
 *  Copyright 2018 Mike Reed
 */

#include "image.h"

#include "../include/GFinal.h"
#include "../include/GCanvas.h"
#include "../include/GBitmap.h"
#include "../include/GColor.h"
#include "../include/GMatrix.h"
#include "../include/GPath.h"
#include "../include/GPoint.h"
#include "../include/GRandom.h"
#include "../include/GRect.h"
#include <string>

static GPoint center(const GRect& r) {
    return { (r.left + r.right)*0.5f, (r.top + r.bottom)*0.5f };
}

static void draw_rect_radial(GFinal* fin, GCanvas* canvas, const GRect& r, float radius,
                             const GColor colors[], int count, GShader::TileMode mode) {
    auto sh = fin->createRadialGradient(center(r), radius, colors, count, mode);
    if (sh) {
        canvas->drawRect(r, GPaint(sh.get()));
    }
}

static void final_radial(GCanvas* canvas) {
    auto fin = GCreateFinal();

    const GColor c0[] = { {1,0,0,1}, {0,1,0,1}, {0,0,1,1} };
    const GColor c1[] = { {0,0,0,1}, {1,1,1,1} };

    struct {
        GPoint              offset;
        float               radius;
        float               scale;
        GShader::TileMode   mode;
        const GColor*       colors;
        int                 count;
    } recs[] = {
        { {-256, -256},  90, 4, GShader::kClamp,  c1, GARRAY_COUNT(c1) },
        { {   0,    0}, 120, 1, GShader::kClamp,  c0, GARRAY_COUNT(c0) },
        { { 256,    0},  60, 1, GShader::kRepeat, c0, GARRAY_COUNT(c0) },
        { {   0,  256},  60, 1, GShader::kMirror, c0, GARRAY_COUNT(c0) },
    };
    for (auto& r : recs) {
        canvas->save();
        canvas->translate(r.offset.x, r.offset.y);
        canvas->scale(r.scale, r.scale);
        draw_rect_radial(fin.get(), canvas, GRect::LTRB(0, 0, 256, 256), r.radius,
                         r.colors, r.count, r.mode);
        canvas->restore();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

static void final_coons(GCanvas* canvas) {
    GBitmap bm;
    bm.readFromFile("apps/spock.png");
    assert(bm.width());
    assert(bm.height());

    GPoint pts[] = {
        {0, 0}, {0.25f, 0.5}, {1, 0},
                              {1.25, 0.5f},
                              {0.75, 1},
                {0.5f, 0.75},
        {0, 1.125},
        {0.25, 0.5f},
    };
    auto mx = GMatrix::Translate(30, 30) * GMatrix::Scale(400, 400);
    mx.mapPoints(pts, pts, 8);

    GPoint tex[] = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1},
    };
    GMatrix::Scale(bm.width(), bm.height()).mapPoints(tex, tex, 4);

    auto sh = GCreateBitmapShader(bm, GMatrix());
    GPaint paint(sh.get());
    assert(sh.get());

    const int N = 8;
    GCreateFinal()->drawQuadraticCoons(canvas, pts, tex, N, paint);
}

////////

static GColorMatrix scale(float s) {
    return GColorMatrix({s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1, 0, 0, 0, 0});
}

static GColorMatrix gray() {
    const float sr = 0.299f;
    const float sg = 0.587f;
    const float sb = 0.114f;
    return GColorMatrix({sr, sr, sr, 0,
                         sg, sg, sg, 0,
                         sb, sb, sb, 0,
                          0,  0,  0, 1,
                          0,  0,  0, 0});
}

static void draw_cm(GFinal* f, GCanvas* canvas, const GRect& r, const GColorMatrix& cm,
                    GShader* shader) {
    if (auto cmShader = f->createColorMatrixShader(cm, shader)) {
        GPaint paint;
        paint.setShader(cmShader.get());
        canvas->drawRect(r, paint);
    }
}

static std::unique_ptr<GShader> make_bm_shader(const char path[], float w, float h) {
    GBitmap bm;
    bm.readFromFile(path);
    assert(bm.width());
    assert(bm.height());
    return GCreateBitmapShader(bm, GMatrix::Scale(bm.width()/w, bm.height()/h));
}

static void final_colormarix(GCanvas* canvas) {
    auto f = GCreateFinal();
    const float W = 512.0f/3;
    const float H = 512.0f/3;
    const GRect r = {0, 0, W, H};

    const GColor colors[] = {{1,0,0,1}, {0,1,0,0}, {0,0,1,1}};
    auto sh0 = make_bm_shader("apps/spock.png", W, H);
    auto sh1 = make_bm_shader("apps/wheel.png", W, H);
    auto sh2 = GCreateLinearGradient({0, 0}, {W, H}, colors, 3);
    GShader* const shaders[] = {sh0.get(), sh1.get(), sh2.get()};

    const GColorMatrix cms[] = {
        scale(2),
        [](){ auto cm = scale(-1); cm[16] = cm[17] = cm[18] = 1; return cm;}(),
        gray(),
    };

    for (auto sh : shaders) {
        canvas->save();
        for (const auto& cm : cms) {
            draw_cm(f.get(), canvas, r, cm, sh);
            canvas->translate(W, 0);
        }
        canvas->restore();
        canvas->translate(0, H);
    }
}

//////////////////////////////////////////

static void make_star(std::vector<GPoint>& pts, int count, float anglePhase) {
    assert(count & 1);
    float da = 2 * gFloatPI * (count >> 1) / count;
    float angle = anglePhase;
    for (int i = 0; i < count; ++i) {
        pts.push_back({ cosf(angle), sinf(angle) });
        angle += da;
    }
}

std::vector<GPoint> make_wiggle(float scalex, float scaley) {
    float limit = 4 * gFloatPI;
    float da = limit / 40;
    std::vector<GPoint> pts;
    for (float a = 0; a <= limit; a += da) {
        pts.push_back({std::sin(a) * scalex, a * scaley});
    }
    return pts;
}

static void final_stroke(GCanvas* canvas) {
    auto f = GCreateFinal();

    GPaint paint;
    float w = 20;

    paint.setColor({1, 0, 0, 1});
    float dx = 120, dy = 90;
    const GPoint pts[] = {
        {dx, dy}, {512-dx, 512-dy}, {512-dx,dy}, {dx,512-dy},
    };
    canvas->drawPath(f->strokePolygon(pts, 4, 70, true), paint);
    paint.setBlendMode(GBlendMode::kClear);
    canvas->drawPath(f->strokePolygon(pts, 4, 25, true), paint);
    paint.setBlendMode(GBlendMode::kSrcOver);

    std::vector<GPoint> poly;
    make_star(poly, 7, gFloatPI/14);
    w = 15;
    canvas->save();
    canvas->translate(256, 310);
    canvas->scale(190,  190);
    paint.setColor({0,0,1,1});
    canvas->drawPath(f->strokePolygon(poly.data(), poly.size(), w/256, true), paint);
    canvas->restore();

    poly = make_wiggle(25, 30);
    canvas->save();
    canvas->translate(40, 75);
    paint.setColor({0,1,0,1});
    auto path = f->strokePolygon(poly.data(), poly.size(), 15, false);
    canvas->drawPath(path, paint);
    paint.setColor({1,0,1,1});
    canvas->translate(432, 0);
    canvas->drawPath(path, paint);
    canvas->restore();
}
