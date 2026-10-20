#pragma once

#include "core/Image2D.h"

#include <QVector>

namespace iat {

struct Histogram {
    double lo = 0.0;           // left edge of the first bin
    double hi = 1.0;           // right edge of the last bin
    double dataMin = 0.0;      // smallest finite pixel value
    double dataMax = 0.0;      // largest finite pixel value
    QVector<qint64> counts;
    qint64 maxCount = 0;

    bool isEmpty() const { return counts.isEmpty(); }
    int binCount() const { return int(counts.size()); }
    double binWidth() const { return counts.isEmpty() ? 0.0 : (hi - lo) / double(counts.size()); }
};

// Histogram of the finite pixels of `image`.
//
// For whole-number images (raw sensor data) every bin covers the same number
// of integer values, which avoids the comb-like artefacts you get when bin
// edges fall between integers.
Histogram computeHistogram(const Image2D& image, const ImageStats& stats, int targetBins = 512);

} // namespace iat
