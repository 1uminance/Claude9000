#include "ImageProcessor.h"

namespace FilmTools {

void ImageProcessor::process(const OfxRectI& renderWindow) {
    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        if (dstImg_->getPixelAddress(renderWindow.x1, y) == nullptr)
            continue; // row outside allocated region
        processRow(y, renderWindow.x1, renderWindow.x2);
    }
}

} // namespace FilmTools
