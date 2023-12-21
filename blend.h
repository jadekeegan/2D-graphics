/*
 *  Copyright 2023 Jade Keegan
 */

#ifndef blend_DEFINED
#define blend_DEFINED

#include "include/GCanvas.h"
#include "include/GRect.h"
#include "include/GColor.h"
#include "include/GBitmap.h"
#include "helpers.h"

GPixel clearMode(GPixel src, GPixel dst);
GPixel srcMode(GPixel src, GPixel dst);
GPixel dstMode(GPixel src, GPixel dst);
GPixel srcOver(GPixel src, GPixel dst);
GPixel dstOver(GPixel src, GPixel dst);
GPixel srcIn(GPixel src, GPixel dst);
GPixel dstIn(GPixel src, GPixel dst);
GPixel srcOut(GPixel src, GPixel dst);
GPixel dstOut(GPixel src, GPixel dst);
GPixel srcATop(GPixel src, GPixel dst);
GPixel dstATop(GPixel src, GPixel dst);
GPixel xorMode(GPixel src, GPixel dst);

// Blend Helpers
GPixel clearMode(GPixel src, GPixel dst) {
    return GPixel_PackARGB(0,0,0,0);
}

// kSrc
GPixel srcMode(GPixel src, GPixel dst) {
    return src;
}

// kDst
GPixel dstMode(GPixel src, GPixel dst) {
    return dst;
}

// kSrcOver
GPixel srcOver(GPixel src, GPixel dst) {
    return src + dstOut(src, dst);      
}

// kDstOver
GPixel dstOver(GPixel src, GPixel dst) {
    return dst + srcOut(src, dst);     
}

// kSrcIn
GPixel srcIn(GPixel src, GPixel dst) {
    const int dst_a = GPixel_GetA(dst);

    return quad_mul_div255(src, dst_a);
}

// kDstIn
GPixel dstIn(GPixel src, GPixel dst) {
    const int src_a = GPixel_GetA(src);

    return quad_mul_div255(dst, src_a);
}

// kSrcOut
GPixel srcOut(GPixel src, GPixel dst) {
    return quad_mul_div255(src, 255 - GPixel_GetA(dst));
}

// kDstOut
GPixel dstOut(GPixel src, GPixel dst) {
    return quad_mul_div255(dst, 255 - GPixel_GetA(src));
}

// kSrcATop
GPixel srcATop(GPixel src, GPixel dst) {
    return srcIn(src, dst) + dstOut(src, dst);
}

// kDstATop
GPixel dstATop(GPixel src, GPixel dst) {
    return dstIn(src, dst) + srcOut(src, dst);
}

// kXor
GPixel xorMode(GPixel src, GPixel dst) {
    return dstOut(src, dst) + srcOut(src, dst);
}

// Blend Modes (src = new color, dst = old pixel)
typedef GPixel (*BlendProc)(GPixel, GPixel);

const BlendProc gProcs[] = {
// since our enum values range from 0 … 11, we can prepopulate
// an array with their corresponding function values.
    clearMode, srcMode, dstMode, srcOver, dstOver, srcIn, dstIn, srcOut, dstOut, srcATop, dstATop, xorMode
};


BlendProc findBlendProc(GShader* shader, GBlendMode mode, float alpha) {
    BlendProc blend = gProcs[(int)mode];

    if (alpha == 0 && (mode == GBlendMode::kSrcIn || mode == GBlendMode::kDstIn || mode == GBlendMode::kSrcOut || mode == GBlendMode::kDstATop)) {
        blend = gProcs[(int)GBlendMode::kClear];
    }
    
    return blend;
}

#endif