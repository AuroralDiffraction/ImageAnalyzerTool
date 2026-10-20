#include "core/Image2D.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace iat {

Image2D::Image2D(int width, int height, double fill)
    : m_width(std::max(0, width))
    , m_height(std::max(0, height))
    , m_data(qsizetype(m_width) * m_height, fill)
{
    if (m_data.isEmpty())
        m_width = m_height = 0;
}

ImageStats Image2D::computeStats() const
{
    ImageStats s;
    s.min = std::numeric_limits<double>::infinity();
    s.max = -std::numeric_limits<double>::infinity();

    double sum = 0.0;
    for (double v : m_data) {
        if (!std::isfinite(v)) {
            ++s.nonFiniteCount;
            continue;
        }
        ++s.finiteCount;
        sum += v;
        if (v < s.min) s.min = v;
        if (v > s.max) s.max = v;
        if (s.integerValued && v != std::floor(v))
            s.integerValued = false;
    }

    if (s.finiteCount == 0) {
        s.min = s.max = s.mean = 0.0;
        s.integerValued = false;
        return s;
    }

    s.mean = sum / double(s.finiteCount);
    return s;
}

} // namespace iat
