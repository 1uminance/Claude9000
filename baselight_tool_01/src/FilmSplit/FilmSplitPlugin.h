#pragma once
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ImageProcessor.h"

namespace FilmTools {

// ── Pixel processor ────────────────────────────────────────────────────────────
// Extracts one channel and outputs it as a monochrome image (R=G=B=channel_value).
class FilmSplitProcessor : public ImageProcessor {
public:
    int   channel     = 0;    // 0=R, 1=G, 2=B
    float sensitivity = 1.0f; // multiplier applied before output

protected:
    void processRow(int y, int x1, int x2) override;
};

// ── Plugin ─────────────────────────────────────────────────────────────────────
class FilmSplitPlugin : public OFX::ImageEffect {
public:
    explicit FilmSplitPlugin(OfxImageEffectHandle handle);

    void render(const OFX::RenderArguments& args) override;
    bool isIdentity(const OFX::IsIdentityArguments& args,
                    OFX::Clip*& identityClip,
                    double& identityTime) override;

private:
    OFX::Clip*         srcClip_          = nullptr;
    OFX::Clip*         dstClip_          = nullptr;
    OFX::ChoiceParam*  channelParam_     = nullptr;
    OFX::DoubleParam*  sensitivityParam_ = nullptr;

    void setupAndProcess(FilmSplitProcessor& proc, const OFX::RenderArguments& args);
};

// ── Factory ────────────────────────────────────────────────────────────────────
class FilmSplitPluginFactory
    : public OFX::PluginFactoryHelper<FilmSplitPluginFactory> {
public:
    FilmSplitPluginFactory()
        : OFX::PluginFactoryHelper<FilmSplitPluginFactory>(
              "com.filmtools.FilmSplit", 1, 0) {}

    void load() override {}
    void unload() override {}
    void describe(OFX::ImageEffectDescriptor& desc) override;
    void describeInContext(OFX::ImageEffectDescriptor& desc,
                           OFX::ContextEnum context) override;
    OFX::ImageEffect* createInstance(OfxImageEffectHandle handle,
                                     OFX::ContextEnum context) override;
};

} // namespace FilmTools
