#include "FilmCombinePlugin.h"
#include "ofxsParam.h"
#include <stdexcept>

namespace FilmTools {

// ── CrosstalkMatrix ────────────────────────────────────────────────────────────

void CrosstalkMatrix::apply(float& r, float& g, float& b) const {
    // Compute outputs before overwriting inputs
    float out_r = r             + static_cast<float>(GR) * g + static_cast<float>(BR) * b;
    float out_g = static_cast<float>(RG) * r + g             + static_cast<float>(BG) * b;
    float out_b = static_cast<float>(RB) * r + static_cast<float>(GB) * g + b;
    r = out_r;
    g = out_g;
    b = out_b;
}

// ── FilmCombineProcessor ───────────────────────────────────────────────────────

void FilmCombineProcessor::processRow(int y, int x1, int x2) {
    for (int x = x1; x < x2; ++x) {
        float* d = dstPix(x, y);
        if (!d) continue;

        const float* r = rPix(x, y);
        const float* g = gPix(x, y);
        const float* b = bPix(x, y);

        // Each input is monochrome (R=G=B=channel_value from FilmSplit).
        // We read component [0] as the channel value.
        float rv = r ? r[0] * gainR : 0.0f;
        float gv = g ? g[0] * gainG : 0.0f;
        float bv = b ? b[0] * gainB : 0.0f;

        matrix.apply(rv, gv, bv);

        d[0] = rv;
        d[1] = gv;
        d[2] = bv;
        // Alpha: take from R channel input if available, else 1.0
        d[3] = (nComponents_ == 4) ? (r ? r[3] : 1.0f) : 1.0f;
    }
}

// ── FilmCombinePlugin ──────────────────────────────────────────────────────────

FilmCombinePlugin::FilmCombinePlugin(OfxImageEffectHandle handle)
    : OFX::ImageEffect(handle) {
    rClip_   = fetchClip("R_Channel");
    gClip_   = fetchClip("G_Channel");
    bClip_   = fetchClip("B_Channel");
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);

    paramRG_ = fetchDoubleParam("xtalk_RG");
    paramRB_ = fetchDoubleParam("xtalk_RB");
    paramGR_ = fetchDoubleParam("xtalk_GR");
    paramGB_ = fetchDoubleParam("xtalk_GB");
    paramBR_ = fetchDoubleParam("xtalk_BR");
    paramBG_ = fetchDoubleParam("xtalk_BG");

    paramGainR_ = fetchDoubleParam("gain_R");
    paramGainG_ = fetchDoubleParam("gain_G");
    paramGainB_ = fetchDoubleParam("gain_B");
}

CrosstalkMatrix FilmCombinePlugin::fetchCrosstalkMatrix(double time) const {
    CrosstalkMatrix m;
    paramRG_->getValueAtTime(time, m.RG);
    paramRB_->getValueAtTime(time, m.RB);
    paramGR_->getValueAtTime(time, m.GR);
    paramGB_->getValueAtTime(time, m.GB);
    paramBR_->getValueAtTime(time, m.BR);
    paramBG_->getValueAtTime(time, m.BG);
    return m;
}

void FilmCombinePlugin::setupAndProcess(FilmCombineProcessor& proc,
                                         const OFX::RenderArguments& args) {
    std::unique_ptr<OFX::Image> rImg(rClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> gImg(gClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> bImg(bClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));

    if (!rImg || !gImg || !bImg || !dst)
        throw std::runtime_error("FilmCombine: one or more inputs are not connected");

    if (dst->getPixelDepth() != OFX::eBitDepthFloat)
        throw std::runtime_error("FilmCombine: only 32-bit float is supported");

    OFX::PixelComponentEnum dstComponents = dst->getPixelComponents();
    proc.nComponents_ = (dstComponents == OFX::ePixelComponentRGBA) ? 4 : 3;

    proc.rImg_   = rImg.get();
    proc.gImg_   = gImg.get();
    proc.bImg_   = bImg.get();
    proc.dstImg_ = dst.get();

    proc.matrix = fetchCrosstalkMatrix(args.time);

    double gainR = 1.0, gainG = 1.0, gainB = 1.0;
    paramGainR_->getValueAtTime(args.time, gainR);
    paramGainG_->getValueAtTime(args.time, gainG);
    paramGainB_->getValueAtTime(args.time, gainB);
    proc.gainR = static_cast<float>(gainR);
    proc.gainG = static_cast<float>(gainG);
    proc.gainB = static_cast<float>(gainB);

    proc.process(args.renderWindow);
}

void FilmCombinePlugin::render(const OFX::RenderArguments& args) {
    FilmCombineProcessor proc;
    setupAndProcess(proc, args);
}

bool FilmCombinePlugin::isIdentity(const OFX::IsIdentityArguments& args,
                                    OFX::Clip*& identityClip,
                                    double& identityTime) {
    CrosstalkMatrix m = fetchCrosstalkMatrix(args.time);
    double gainR = 1.0, gainG = 1.0, gainB = 1.0;
    paramGainR_->getValueAtTime(args.time, gainR);
    paramGainG_->getValueAtTime(args.time, gainG);
    paramGainB_->getValueAtTime(args.time, gainB);

    bool xtalkZero = (m.RG == 0 && m.RB == 0 && m.GR == 0 &&
                      m.GB == 0 && m.BR == 0 && m.BG == 0);
    bool gainsUnity = (gainR == 1.0 && gainG == 1.0 && gainB == 1.0);

    // Even with identity params, combine is never a passthrough because
    // its inputs are separate monochrome clips, not the original RGB source.
    (void)identityClip; (void)identityTime;
    (void)xtalkZero; (void)gainsUnity;
    return false;
}

// ── FilmCombinePluginFactory ───────────────────────────────────────────────────

void FilmCombinePluginFactory::describe(OFX::ImageEffectDescriptor& desc) {
    desc.setLabels("Film Combine", "Film Combine", "Film Combine");
    desc.setPluginGrouping("FilmTools");
    desc.setPluginDescription(
        "Recombines three independently processed monochrome channel streams "
        "(outputs of Film Split) back into a single RGB image. Applies a "
        "cross-talk matrix to simulate dye-layer interlayer diffusion in film.");

    desc.addSupportedContext(OFX::eContextGeneral);
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

void FilmCombinePluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc,
                                                  OFX::ContextEnum /*context*/) {
    // ── Input clips ────────────────────────────────────────────────────────────
    for (const char* name : {"R_Channel", "G_Channel", "B_Channel"}) {
        OFX::ClipDescriptor* clip = desc.defineClip(name);
        clip->addSupportedComponent(OFX::ePixelComponentRGBA);
        clip->addSupportedComponent(OFX::ePixelComponentRGB);
        clip->setSupportsTiles(true);
        clip->setOptional(false);
    }

    OFX::ClipDescriptor* dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGB);
    dstClip->setSupportsTiles(true);

    // ── Cross-talk parameters ──────────────────────────────────────────────────
    // Naming: XY = "X channel bleeds into Y output"
    // Typical film values: 0.00 – 0.08. Negative = colour subtraction.
    auto defineCrosstalk = [&](const char* name, const char* label,
                                const char* hint, double defaultVal) {
        OFX::DoubleParamDescriptor* p = desc.defineDoubleParam(name);
        p->setLabel(label);
        p->setHint(hint);
        p->setRange(-1.0, 1.0);
        p->setDisplayRange(-0.15, 0.15);
        p->setDefault(defaultVal);
        p->setIncrement(0.001);
        p->setAnimates(true);
        p->setDoubleType(OFX::eDoubleTypeScale);
    };

    defineCrosstalk("xtalk_RG", "R → G",
        "How much of the Red channel bleeds into the Green output. "
        "Positive = warm contamination of the green layer.", 0.0);
    defineCrosstalk("xtalk_RB", "R → B",
        "How much of the Red channel bleeds into the Blue output.", 0.0);
    defineCrosstalk("xtalk_GR", "G → R",
        "How much of the Green channel bleeds into the Red output.", 0.0);
    defineCrosstalk("xtalk_GB", "G → B",
        "How much of the Green channel bleeds into the Blue output.", 0.0);
    defineCrosstalk("xtalk_BR", "B → R",
        "How much of the Blue channel bleeds into the Red output.", 0.0);
    defineCrosstalk("xtalk_BG", "B → G",
        "How much of the Blue channel bleeds into the Green output.", 0.0);

    // ── Output gain parameters ─────────────────────────────────────────────────
    auto defineGain = [&](const char* name, const char* label) {
        OFX::DoubleParamDescriptor* p = desc.defineDoubleParam(name);
        p->setLabel(label);
        p->setHint("Per-channel output gain applied after cross-talk.");
        p->setRange(0.0, 4.0);
        p->setDisplayRange(0.5, 2.0);
        p->setDefault(1.0);
        p->setIncrement(0.01);
        p->setAnimates(true);
        p->setDoubleType(OFX::eDoubleTypeScale);
    };

    defineGain("gain_R", "Gain R");
    defineGain("gain_G", "Gain G");
    defineGain("gain_B", "Gain B");
}

OFX::ImageEffect* FilmCombinePluginFactory::createInstance(
    OfxImageEffectHandle handle, OFX::ContextEnum /*context*/) {
    return new FilmCombinePlugin(handle);
}

} // namespace FilmTools
