#pragma once
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ImageProcessor.h"

namespace FilmTools {

// ── Cross-talk matrix ──────────────────────────────────────────────────────────
// Models dye-layer interlayer diffusion in film.
// Naming convention: RG = R bleeds into G output (source_destination).
//
// Full matrix applied as:  out = M * in
//
//   out_R = R           + m_GR * G  + m_BR * B
//   out_G = m_RG * R    + G         + m_BG * B
//   out_B = m_RB * R    + m_GB * G  + B
//
// Off-diagonal entries are typically small positive values (0.0 – 0.10).
// Negative values (colour subtraction) are valid but unusual.
struct CrosstalkMatrix {
    double RG = 0.0; // R → G
    double RB = 0.0; // R → B
    double GR = 0.0; // G → R
    double GB = 0.0; // G → B
    double BR = 0.0; // B → R
    double BG = 0.0; // B → G

    void apply(float& r, float& g, float& b) const;
};

// ── Pixel processor ────────────────────────────────────────────────────────────
// Reads the R channel from each of the three monochrome inputs (output of
// FilmSplit), applies per-channel gain, cross-talk, then writes RGB output.
class FilmCombineProcessor : public ImageProcessor {
public:
    OFX::Image* rImg_ = nullptr; // processed Red stream
    OFX::Image* gImg_ = nullptr; // processed Green stream
    OFX::Image* bImg_ = nullptr; // processed Blue stream

    CrosstalkMatrix matrix;
    float gainR = 1.0f;
    float gainG = 1.0f;
    float gainB = 1.0f;

protected:
    void processRow(int y, int x1, int x2) override;

private:
    const float* rPix(int x, int y) const {
        return rImg_ ? static_cast<const float*>(rImg_->getPixelAddress(x, y)) : nullptr;
    }
    const float* gPix(int x, int y) const {
        return gImg_ ? static_cast<const float*>(gImg_->getPixelAddress(x, y)) : nullptr;
    }
    const float* bPix(int x, int y) const {
        return bImg_ ? static_cast<const float*>(bImg_->getPixelAddress(x, y)) : nullptr;
    }
};

// ── Plugin ─────────────────────────────────────────────────────────────────────
class FilmCombinePlugin : public OFX::ImageEffect {
public:
    explicit FilmCombinePlugin(OfxImageEffectHandle handle);

    void render(const OFX::RenderArguments& args) override;
    bool isIdentity(const OFX::IsIdentityArguments& args,
                    OFX::Clip*& identityClip,
                    double& identityTime) override;

private:
    // Three monochrome input clips — one per processed channel
    OFX::Clip* rClip_   = nullptr; // "R_Channel"
    OFX::Clip* gClip_   = nullptr; // "G_Channel"
    OFX::Clip* bClip_   = nullptr; // "B_Channel"
    OFX::Clip* dstClip_ = nullptr;

    // Cross-talk off-diagonal entries (6 params)
    OFX::DoubleParam* paramRG_ = nullptr;
    OFX::DoubleParam* paramRB_ = nullptr;
    OFX::DoubleParam* paramGR_ = nullptr;
    OFX::DoubleParam* paramGB_ = nullptr;
    OFX::DoubleParam* paramBR_ = nullptr;
    OFX::DoubleParam* paramBG_ = nullptr;

    // Per-channel output gains (3 params)
    OFX::DoubleParam* paramGainR_ = nullptr;
    OFX::DoubleParam* paramGainG_ = nullptr;
    OFX::DoubleParam* paramGainB_ = nullptr;

    void setupAndProcess(FilmCombineProcessor& proc, const OFX::RenderArguments& args);
    CrosstalkMatrix fetchCrosstalkMatrix(double time) const;
};

// ── Factory ────────────────────────────────────────────────────────────────────
class FilmCombinePluginFactory
    : public OFX::PluginFactoryHelper<FilmCombinePluginFactory> {
public:
    FilmCombinePluginFactory()
        : OFX::PluginFactoryHelper<FilmCombinePluginFactory>(
              "com.filmtools.FilmCombine", 1, 0) {}

    void load() override {}
    void unload() override {}
    void describe(OFX::ImageEffectDescriptor& desc) override;
    void describeInContext(OFX::ImageEffectDescriptor& desc,
                           OFX::ContextEnum context) override;
    OFX::ImageEffect* createInstance(OfxImageEffectHandle handle,
                                     OFX::ContextEnum context) override;
};

} // namespace FilmTools
