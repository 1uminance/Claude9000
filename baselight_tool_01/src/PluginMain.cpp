#include "ofxsImageEffect.h"
#include "FilmSplitPlugin.h"
#include "FilmCombinePlugin.h"

namespace OFX {
namespace Plugin {

void getPluginIDs(OFX::PluginFactoryArray& ids)
{
    static FilmTools::FilmSplitPluginFactory  filmSplit;
    static FilmTools::FilmCombinePluginFactory filmCombine;
    ids.push_back(&filmSplit);
    ids.push_back(&filmCombine);
}

} // namespace Plugin
} // namespace OFX
