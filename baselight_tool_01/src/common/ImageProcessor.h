#pragma once
#include "ofxsImageEffect.h"

namespace FilmTools {

// Base class for single-pass CPU pixel processors.
// Subclasses implement processRow() and set the image pointers before calling process().
class ImageProcessor {
public:
    virtual ~ImageProcessor() = default;

    // Call after setting all image pointers and parameters.
    void process(const OfxRectI& renderWindow);

    // Primary source / destination. Subclasses may add extra source images.
    OFX::Image* srcImg_ = nullptr;
    OFX::Image* dstImg_ = nullptr;

    // Number of components in the destination image (3 or 4).
    int nComponents_ = 4;

protected:
    // Subclass implements per-row logic.
    virtual void processRow(int y, int x1, int x2) = 0;

    // Safe pixel address helpers — return nullptr if (x,y) is outside the image bounds.
    const float* srcPix(int x, int y) const {
        return srcImg_ ? static_cast<const float*>(srcImg_->getPixelAddress(x, y)) : nullptr;
    }
    float* dstPix(int x, int y) const {
        return dstImg_ ? static_cast<float*>(dstImg_->getPixelAddress(x, y)) : nullptr;
    }
};

} // namespace FilmTools
