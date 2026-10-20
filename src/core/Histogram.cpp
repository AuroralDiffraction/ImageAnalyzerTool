#include "core/Histogram.h"

#include <algorithm>
#include <cmath>

namespace iat {

Histogram computeHistogram(const Image2D& image, const ImageStats& stats, int targetBins)
{
    Histogram h;
    if (!stats.isValid() || targetBins < 1)
        return h;

    h.dataMin = stats.min;
    h.dataMax = stats.max;

    int bins = targetBins;
    const double span = stats.max - stats.min;
    if (stats.integerValued && span < 1e9) {
        // Integer data: bin width is a whole number, bin edges sit at x.5.
        const double distinctValues = span + 1.0;
        const double width = std::max(1.0, std::ceil(distinctValues / targetBins));
        bins = int(std::ceil(distinctValues / width));
        h.lo = stats.min - 0.5;
        h.hi = h.lo + bins * width;
    } else {
        h.lo = stats.min;
        h.hi = stats.max;
        if (!(h.hi > h.lo)) {
            h.lo -= 0.5;
            h.hi += 0.5;
        }
    }

    h.counts.fill(0, bins);
    const double scale = bins / (h.hi - h.lo);
    const double* p = image.constData();
    const qsizetype n = image.pixelCount();
    qint64* counts = h.counts.data();
    for (qsizetype i = 0; i < n; ++i) {
        const double v = p[i];
        if (!std::isfinite(v))
            continue;
        int b = int((v - h.lo) * scale);
        b = std::clamp(b, 0, bins - 1);
        ++counts[b];
    }
    h.maxCount = *std::max_element(h.counts.cbegin(), h.counts.cend());
    return h;
}

} // namespace iat
