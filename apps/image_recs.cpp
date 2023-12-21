/**
 *  Copyright 2020 Mike Reed
 */

#include "image_final.cpp"

const GDrawRec gDrawRecs[] = {
    { final_radial, 512, 512, "final_radial", 7 },
    { final_coons, 512, 512, "final_coons", 7 },
    { final_colormarix, 512, 512, "final_colormatrix", 7 },
    { final_stroke, 512, 512, "final_stroke", 7 },

    { nullptr, 0, 0, nullptr },
};
