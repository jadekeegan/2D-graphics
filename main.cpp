/*
 *  Copyright 2023 Jade Keegan
 */

#include "include/GCanvas.h"
#include "include/GRect.h"
#include "include/GColor.h"
#include "include/GShader.h"
#include "include/GPath.h"
#include "blend.h"
#include "edges.h"
#include "triangle_shader.h"
#include "proxy_shader.h"
#include "combined_shader.h"

using namespace std;
#include <iostream>
#include <stack>

class MyCanvas : public GCanvas {
public:
    MyCanvas(const GBitmap& device) : fDevice(device) {
        matrixStack.push(GMatrix());
    }

    void clear(const GColor& color) override {
        // package src color into a gpixel
        const GPixel& s = GPixel_PackARGB((int) myRound(color.a * 255.f), 
                                            (int) myRound(color.r * color.a * 255.f), 
                                            (int) myRound(color.g * color.a * 255.f), 
                                            (int) myRound(color.b * color.a * 255.f));

        // for loop bounds
        const int height = fDevice.height();
        const int width = fDevice.width();

        // bitmap
        GPixel* pixels = fDevice.pixels();

        // reassign pixels
        for (int y=0; y < height; y++) {
            int y_shift = y*width;
            for (int x=0; x < width; x++) {
                pixels[y_shift+x] = s;
            }
        }
    }
    
    void drawRect(const GRect& rect, const GPaint& paint) override {
        GPoint points[4] = {
            { rect.left, rect.bottom },
            { rect.left, rect.top },
            { rect.right, rect.top },
            { rect.right, rect.bottom },
        };

        drawConvexPolygon(points, 4, paint);
    }

    void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) override {
        // map points to matrix
        GPoint mappedPoints[count];
        CTM.mapPoints(mappedPoints, points, count);

        // build edges   
        std::vector<Edge> edges = buildEdges(fDevice, count, mappedPoints);

        if (edges.size() < 2) {
            return;
        }

        // sort edges        
        std::sort(edges.begin(), edges.end(), compareEdges);

        // package src color into a gpixel
        const GPixel& s = colorToPixel(paint.getColor());

        GShader* shader = paint.getShader();

        // get blend
        GBlendMode mode = paint.getBlendMode();
        float alpha = paint.getAlpha();
        
        BlendProc blend = findBlendProc(shader, mode, alpha);
        if (blend == dstMode) {
            return;
        }

        simpleScan(edges, shader, blend, s);
    }

    void drawPath(const GPath& path, const GPaint& paint) override {
        // transform path
        GPath mappedPath = path;
        mappedPath.transform(CTM);

        // build edges   
        std::vector<Edge> edges = buildPathEdges(mappedPath, fDevice.width(), fDevice.height());

        if (edges.size() < 2) {
            return;
        }
        
        // sort edges        
        std::sort(edges.begin(), edges.end(), compareEdges);

        // package src color into a gpixel
        const GPixel& s = colorToPixel(paint.getColor());

        GShader* shader = paint.getShader();

        // get blend
        GBlendMode mode = paint.getBlendMode();
        float alpha = paint.getAlpha();
        
        BlendProc blend = findBlendProc(shader, mode, alpha);

        if (blend == dstMode) {
            return;
        }

        complexScan(edges, shader, blend, s);
    }

    void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[], int count, const int indices[], const GPaint& paint) override {
        int n = 0;
        
        GShader* shader = paint.getShader();

        for (int i = 0; i < count; ++i) {
            const GPoint trianglePts[3] = { verts[indices[n+0]], verts[indices[n+1]], verts[indices[n+2]] };

            if (texs != nullptr && colors != nullptr) {
                const GColor triangleColors[3] = { colors[indices[n+0]], colors[indices[n+1]], colors[indices[n+2]] };
                const GPoint texPoints[3] = { texs[indices[n+0]], texs[indices[n+1]], texs[indices[n+2]] };
                drawCombinedTriangle(trianglePts, triangleColors, texPoints, shader);

            } else if (colors != nullptr) {
                const GColor triangleColors[3] = { colors[indices[n+0]], colors[indices[n+1]], colors[indices[n+2]] };
                drawTriangle(trianglePts, GPaint(new TriangleShader(trianglePts, triangleColors)));

            } else if (texs != nullptr & shader != nullptr) {
                const GPoint texPoints[3] = { texs[indices[n+0]], texs[indices[n+1]], texs[indices[n+2]] };
                drawTriangleWithTex(trianglePts, texPoints, shader);
            }

            n += 3;
        }
    }

    void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4], int level, const GPaint& paint) override {
        GPoint dividedVerts[4];
        GColor dividedColors[4];
        GPoint dividedTexs[4];

        // add lerp / bilerp
        // lerp: a + x(b-a)
    
        for (int u=0; u<=level; u++) {
                float u0 = (float) u / (float) (level + 1.0f);
                float u1 = (float) (u+1) / (float) (level + 1.0f);

            for (int v=0; v<=level; v++) {
                float v0 = (float) v / (float) (level + 1.0f);
                float v1 = (float) (v+1) / (float) (level + 1.0f);

                dividedVerts[0] = getDividedPoint(verts, u0, v0);
                dividedVerts[1] = getDividedPoint(verts, u1, v0);
                dividedVerts[2] = getDividedPoint(verts, u1, v1);
                dividedVerts[3] = getDividedPoint(verts, u0, v1);

                if (colors != nullptr) {
                    dividedColors[0] = getDividedColor(colors, u0, v0);
                    dividedColors[1] = getDividedColor(colors, u1, v0);
                    dividedColors[2] = getDividedColor(colors, u1, v1);
                    dividedColors[3] = getDividedColor(colors, u0, v1);
                }

                if (texs != nullptr) {
                    dividedTexs[0] = getDividedPoint(texs, u0, v0);
                    dividedTexs[1] = getDividedPoint(texs, u1, v0);
                    dividedTexs[2] = getDividedPoint(texs, u1, v1);
                    dividedTexs[3] = getDividedPoint(texs, u0, v1);
                }

                const int indices[6] = { 0, 1, 3, 1, 2, 3};

                if (colors != nullptr && texs != nullptr) {
                    drawMesh(dividedVerts, dividedColors, dividedTexs, 2, indices, paint);
                } else if (colors != nullptr) {
                    drawMesh(dividedVerts, dividedColors, nullptr, 2, indices, paint);
                } else if (texs != nullptr) {
                    drawMesh(dividedVerts, nullptr, dividedTexs, 2, indices, paint);
                } else {
                    drawMesh(dividedVerts, nullptr, nullptr, 2, indices, paint);
                }

            }
        }
    }

    void save() override {
        matrixStack.push(CTM);
    }

    void restore() override {
        CTM = matrixStack.top();
        matrixStack.pop();
    }

    void concat(const GMatrix& matrix)  override {
        GMatrix result = GMatrix::Concat(CTM, matrix);
        CTM = result;
    }

private:
    // Note: we store a copy of the bitmap
    const GBitmap fDevice;
    GMatrix CTM;
    std::stack<GMatrix> matrixStack;

    GPoint getDividedPoint(const GPoint pts[4], float u, float v) {
        return (1 - v) * ((1 - u) * pts[0] + u * pts[1]) +  v * ((1 - u) * pts[3] + u * pts[2]);
    }

    GColor getDividedColor(const GColor colors[4], float u, float v) {
        float a = (1 - v) * ((1 - u) * colors[0].a + u * colors[1].a) + v * ((1 - u) * colors[3].a + u * colors[2].a);
        float r = (1 - v) * ((1 - u) * colors[0].r + u * colors[1].r) + v * ((1 - u) * colors[3].r + u * colors[2].r);
        float g = (1 - v) * ((1 - u) * colors[0].g + u * colors[1].g) + v * ((1 - u) * colors[3].g + u * colors[2].g);
        float b = (1 - v) * ((1 - u) * colors[0].b + u * colors[1].b) + v * ((1 - u) * colors[3].b + u * colors[2].b);

        return GColor::RGBA(r, g, b, a);
    }

    void blit(int xLeft, int xRight, int y, int N, GShader* shader, BlendProc blend, GPixel s) {

        if (shader == nullptr) {
            for (int x = xLeft; x < xRight; x++) {
                GPixel* dst = fDevice.getAddr(x, y);
                *dst = blend(s, *dst);
            }
        } else {
            if (!shader->setContext(CTM)) {
                return;
            }

            std::vector<GPixel> storage(N);  // make room for at least 'count' results

            shader->shadeRow(xLeft, y, N, &storage[0]);

            if (shader->isOpaque()) {
                
                for (int i = 0; i < N; i++) {
                    GPixel* dst = fDevice.getAddr(xLeft + i, y);
                    *dst = storage[i];
                }
            } else {
                for (int i = 0; i < N; i++) {
                    GPixel* dst = fDevice.getAddr(xLeft + i, y);
                    *dst = blend(storage[i], *dst);
                }
            }
        }
    }

    void simpleScan(std::vector<Edge> edges, GShader* shader, BlendProc blend, GPixel s) {
        Edge e0 = edges[0];
        Edge e1 = edges[1];
        int next_index = 2;

        float xLeft = e0.currX;
        float xRight = e1.currX;

        int yMin = edges.front().top;
        int yMax = edges.back().bottom;

        for (int y = yMin; y < yMax; y++) {
            if (e0.bottom == y) {
                e0 = edges[next_index];
                next_index += 1;

                xLeft = e0.currX;
            }

            if (e1.bottom == y) {
                e1 = edges[next_index];
                next_index += 1;

                xRight = e1.currX;
            }

            int N = myRound(xRight)-myRound(xLeft);

            blit(myRound(xLeft), myRound(xRight), y, N, shader, blend, s);
            
            xLeft += e0.m;
            xRight += e1.m;
        }
    }

    void complexScan(std::vector<Edge> edges, GShader* shader, const BlendProc blend, GPixel s) {
        int L, R;
        int y = edges.front().top;
        while (edges.size() > 0) {
            int i = 0;
            int w = 0;

            while (i<edges.size() && isValidEdge(edges[i], y)) {
                if (w == 0) {
                    L = GRoundToInt(edges[i].currX);
                }

                assert(edges[i].wind == 1 || edges[i].wind == -1);

                w += edges[i].wind;

                if (w==0) {
                    R = GRoundToInt(edges[i].currX);
                    blit(L, R, y, R-L, shader, blend, s);
                }

                if (!isValidEdge(edges[i], y+1)) {
                    edges.erase(edges.begin() + i);
                } else {
                    edges[i].currX += edges[i].m;
                    i++;
                }
            }
            
            assert(w == 0);

            y++;

            while (i < edges.size() && isValidEdge(edges[i], y)) {
                i++;
            }

            std::sort(edges.begin(), edges.begin() + i, compareEdgesX);
        }
    }

    void drawTriangle(const GPoint pts[3], GPaint paint) {
        drawConvexPolygon(pts, 3, paint);
    }

    void drawTriangleWithTex(const GPoint pts[3], const GPoint texs[3], GShader* originalShader) {
        GMatrix P, T, invT;
        P = computeBasis(pts);
        T = computeBasis(texs);

        T.invert(&invT);

        ProxyShader proxy(originalShader, P * invT);
        GPaint p(&proxy);

        this->drawTriangle(pts, p);
    }

    void drawCombinedTriangle(const GPoint pts[3], const GColor colors[], const GPoint texs[3], GShader* shader) {
        GMatrix P, T, invT;
        P = computeBasis(pts);
        T = computeBasis(texs);

        T.invert(&invT);
        
        this->drawTriangle(pts, GPaint(new CombinedShader( 
                                        new TriangleShader(pts, colors),
                                        new ProxyShader(shader, P * invT)
                                        )));
    }

    GMatrix computeBasis(const GPoint pts[3]) {
        return { pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x,
                 pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y };
    }
};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new MyCanvas(device));
}

static void make_star(GPath* path, int count, float anglePhase) {
    assert(count & 1);
    float da = (float) 2 * M_PI * (count >> 1) / count;
    float angle = anglePhase;
    for (int i = 0; i < count; i++) {
        GPoint p = { cosf(angle), sinf(angle) };
        i == 0 ? path->moveTo(p) : path->lineTo(p);
        angle += da;
    }
}

static void add_star(GPath* path, int count) {
    if (count & 1) {
        make_star(path, count, 0);
    }
    else {
        count >>= 1;
        make_star(path, count, 0);
        make_star(path, count, M_PI);
    }
}

std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    // as fancy as you like
    // ...
    // canvas->clear(...);s
    // canvas->fillRect(...);
    canvas->clear(GColor::RGB(1,1,1));
    canvas->fillRect(GRect::XYWH(0,0,256,200), GColor::RGBA(0, 0, 0.54f, 0.30f));
    canvas->fillRect(GRect::XYWH(0,215,256,156), GColor::RGB(0.42f, 0.36f, 0.30f));
    canvas->fillRect(GRect::XYWH(0,200,20,20), GColor::RGB(0.36f, 0.51f, 0.34f));
    canvas->fillRect(GRect::XYWH(100,190,30,80), GColor::RGBA(0.11f, 0.26f, 0.20f, 0.90f));
    canvas->fillRect(GRect::XYWH(200,130,55,50), GColor::RGB(0.36f, 0.51f, 0.34f));
    canvas->fillRect(GRect::XYWH(215,0,30,30), GColor::RGB(0.36f, 0.51f, 0.34f));
    canvas->fillRect(GRect::XYWH(60,160,75,60), GColor::RGB(0.20f, 0.31f, 0.25f));
    canvas->fillRect(GRect::XYWH(100,30,50,70), GColor::RGBA(0.11f, 0.26f, 0.20f, 0.90f));
    canvas->fillRect(GRect::XYWH(175,175,30,40), GColor::RGB(0.20f, 0.31f, 0.25f));
    canvas->fillRect(GRect::XYWH(250,250,40,55), GColor::RGB(0.20f, 0.31f, 0.25f));
    canvas->fillRect(GRect::XYWH(240,115,25,40), GColor::RGBA(0.32f, 0.47f, 0.44f, 0.80f));
    canvas->fillRect(GRect::XYWH(0,100,120,100), GColor::RGBA(0.32f, 0.47f, 0.44f, 0.95f));
    canvas->fillRect(GRect::XYWH(135,150,75,85), GColor::RGBA(0.32f, 0.47f, 0.44f, 0.80f));
    canvas->fillRect(GRect::XYWH(65,0,130,125), GColor::RGBA(0.52f, 0.66f, 0.55f, 0.75f));
    canvas->fillRect(GRect::XYWH(150,35,80,60), GColor::RGBA(0.52f, 0.66f, 0.55f, 0.75f));
    canvas->fillRect(GRect::XYWH(35,30,90,60), GColor::RGBA(0.11f, 0.26f, 0.20f, 0.90f));

    GMatrix scale = GMatrix::Scale(5, 5);
    GMatrix scale2 = GMatrix::Scale(15, 15);

    GPath path;;
    add_star(&path, 6);
    path.transform(scale);
    canvas->translate(60, 60);
    canvas->drawPath(path, GPaint({ 1, 1,.90f, 1 }));

    path.reset();
    add_star(&path, 6);
    path.transform(scale);
    canvas->translate(15, 15);
    canvas->drawPath(path, GPaint({ 1, 1,.43f, 1 }));

    path.reset();
    add_star(&path, 6);
    path.transform(scale);
    canvas->translate(10, 5);
    canvas->drawPath(path, GPaint({ 1, 1,.62f, 1 }));

    path.reset();
    add_star(&path, 6);
    path.transform(scale2);
    canvas->translate(45, 30);
    canvas->drawPath(path, GPaint({ 1, 1,.55f, 1 }));

    path.reset();
    add_star(&path, 6);
    path.transform(scale);
    canvas->translate(12, 17);
    canvas->drawPath(path, GPaint({ 1, 1,.53f, 1 }));

    path.reset();
    add_star(&path, 6);
    path.transform(scale);
    canvas->translate(25, 15);
    canvas->drawPath(path, GPaint({ 1, 1,.92f, 1 }));

    path.reset();
    add_star(&path, 6);
    path.transform(scale2);
    canvas->translate(60, 20);
    canvas->drawPath(path, GPaint({ 1, 1,.75f, 1 }));

    path.reset();
    add_star(&path, 5);
    path.transform(scale2);
    canvas->translate(-80, -75);
    canvas->drawPath(path, GPaint({ 1, 1,.79f, 1 }));


    path.reset();
    path.addCircle({0,0}, 40, GPath::kCW_Direction);
    canvas->translate(-130,-75);
    canvas->drawPath(path, GPaint({1, 1, 1, .75f})); 

    return "starry forest";
}