/*
 *  Copyright 2023 <Jade Keegan>
 */

#ifndef helpers_DEFINED
#define helpers_DEFINED

// Helper Functions
int myRound(float x) {
    if (x < 0 ) {
        return 0;
    }

    return (int) floor(x + 0.5f);
}

GPixel colorToPixel(GColor color) {
    return GPixel_PackARGB((int) myRound(color.a * 255.f), 
                            (int) myRound(color.r * color.a * 255.f), 
                            (int) myRound(color.g * color.a * 255.f), 
                            (int) myRound(color.b * color.a * 255.f));
}

// Divide Helpers
// turn 0xAABBCCDD into 0x00AA00CC00BB00DD
uint64_t expand(uint32_t x) {
    uint64_t hi = x & 0xFF00FF00;  // the A and G components
    uint64_t lo = x & 0x00FF00FF;  // the R and B components
    return (hi << 24) | lo;
}

// turn 0xXX into 0x00XX00XX00XX00XX
uint64_t replicate(uint64_t x) {
    return (x << 48) | (x << 32) | (x << 16) | x;
}

// turn 0x..AA..CC..BB..DD into 0xAABBCCDD
uint32_t compact(uint64_t x) {
    return ((x >> 24) & 0xFF00FF00) | (x & 0xFF00FF);
}

uint32_t quad_mul_div255(uint32_t x, uint8_t invA) {
    uint64_t prod = expand(x) * invA;
    prod += replicate(128);			
    prod += (prod >> 8) & replicate(0xFF);
    prod >>= 8;
    return compact(prod);
}

uint32_t quad_mul(uint32_t x, uint8_t a) {
    uint64_t prod = expand(x) * a;
    return compact(prod);
}

#endif /* helpers_DEFINED */