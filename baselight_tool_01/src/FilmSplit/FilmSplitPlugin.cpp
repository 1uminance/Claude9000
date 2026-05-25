#include "FilmSplitPlugin.h"
#include "ofxsParam.h"
#include <stdexcept>

namespace FilmTools {

// ── FilmSplitProcessor ─────────────────────────────────────────────────────────

void FilmSplitProcessor::processRow(int y, int x1, int x2) {
    for (int x = x1; x < x2; ++x) {
        const float* s = srcPix(x, y);
        float*       d = dstPix(x, y);

        if (!d) continue;

        if (s) {
            float val = s[channel] * sensitivity;
            d[0] = val;
            d[1] = val;
            d[2] = val;
            d[3] = (nComponents_ == 4) ? s[3] : 1.0f;
        } else {
            // Source doesn't cover this pixel — output black/transparent
            d[0] = d[1] = d[2] = 0.0f;
            d[3] = 1.0f;
        }
    }
}

// ── FilmSplitPlugin ────────────────────────────────────────────────────────────

FilmSplitPlugin::FilmSplitPlugin(OfxImageEffectHandle handle)
    : OFX::ImageEffect(handle) {
    srcClip_          = fetchClip(kOfxImageEffectSimpleSourceClipName);
    dstClip_          = fetchClip(kOfxImageEffectOutputClipName);
    channelParam_     = fetchChoiceParam("channel");
    sensitivityParam_ = fetchDoubleParam("sensitivity");
}

void FilmSplitPlugin::setupAndProcess(FilmSplitProcessor& proc,
                                       const OFX::RenderArguments& args) {
    std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));

    if (!src || !dst)
        throw std::runtime_error("FilmSplit: failed to fetch images");

    if (dst->getPixelDepth() != OFX::eBitDepthFloat ||
        src->getPixelDepth() != OFX::eBitDepthFloat)
        throw std::runtime_error("FilmSplit: only 32-bit float is supported");

    OFX::PixelComponentEnum dstComponents = dst->getPixelComponents();
    proc.nComponents_ = (dstComponents == OFX::ePixelComponentRGBA) ? 4 : 3;
    proc.srcImg_      = src.get();
    proc.dstImg_      = dst.get();

    int ch = 0;
    channelParam_->getValueAtTime(args.time, ch);
    proc.channel = ch;

    double sensitivity = 1.0;
    sensitivityParam_->getValueAtTime(args.time, sensitivity);
    proc.sensitivity = static_cast<float>(sensitivity);

    proc.process(args.renderWindow);
}

void FilmSplitPlugin::render(const OFX::RenderArguments& args) {
    FilmSplitProcessor proc;
    setupAndProcess(proc, args);
}

bool FilmSplitPlugin::isIdentity(const OFX::IsIdentityArguments& args,
                                  OFX::Clip*& identityClip,
                                  double& identityTime) {
    // FilmSplit always changes the image (converts to monochrome),
    // so it is never identity — even at sensitivity=1.0.
    (void)args; (void)identityClip; (void)identityTime;
    return false;
}

// ── FilmSplitPluginFactory ─────────────────────────────────────────────────────

void FilmSplitPluginFactory::describe(OFX::ImageEffectDescriptor& desc) {
    desc.setLabels("Film Split", "Film Split", "Film Split");
    desc.setPluginGrouping("FilmTools");
    desc.setPluginDescription(
        "Extracts a single RGB channel as a monochrome image for per-channel "
        "film stock processing. Use before grain, halation, or glow operators, "
        "then recombine with Film Combine.");

    desc.addSupportedContext(OFX::eContextFilter);
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);

    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(true);
    desc.setSupportsTiles(true);
    desc.setTemporalClipAccess(false);
    desc.setRenderTwiceAlways(false);
    desc.setSupportsMultipleClipPARs(false);
    desc.setSupportsMultipleClipDepths(false);
}

void FilmSplitPluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc,
                                                OFX::ContextEnum /*context*/) {
    // Source clip
    OFX::ClipDescriptor* srcClip =
        desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGB);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(true);
    srcClip->setIsMask(false);

    // Output clip
    OFX::ClipDescriptor* dstClip =
        desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGB);
    dstClip->setSupportsTiles(true);

    // ── Parameters ────────────────────────────────────────────────────────────

    // Channel selector
    {
        OFX::ChoiceParamDescriptor* p = desc.defineChoiceParam("channel");
        p->setLabel("Channel");
        p->setHint("RGB channel to extract. Each channel feeds a separate "
                   "processing branch before Film Combine.");
        p->appendOption("Red");
        p->appendOption("Green");
        p->appendOption("Blue");
        p->setDefault(0);
        p->setAnimates(false);
    }

    // Sensitivity (per-channel gain)
    {
        OFX::DoubleParamDescriptor* p = desc.defineDoubleParam("sensitivity");
        p->setLabel("Sensitivity");
        p->setHint("Per-channel gain applied on extraction. Use to emulate the "
                   "relative spectral sensitivity of each emulsion layer. "
                   "Film stock presets supply recommended values.");
        p->setRange(0.1, 4.0);
        p->setDisplayRange(0.5, 2.0);
        p->setDefault(1.0);
        p->setIncrement(0.01);
        p->setAnimates(true);
        p->setDoubleType(OFX::eDoubleTypeScale);
    }
}

OFX::ImageEffect* FilmSplitPluginFactory::createInstance(
    OfxImageEffectHandle handle, OFX::ContextEnum /*context*/) {
    return new FilmSplitPlugin(handle);
}

} // namespace FilmTools
