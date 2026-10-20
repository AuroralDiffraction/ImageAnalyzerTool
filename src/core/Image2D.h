#pragma once

#include <QVector>
#include <QtGlobal>

namespace iat {

// Statistics of the finite pixels, used for the histogram and the contrast
// range. (The Mean / Std shown in the panels follow MATLAB's definitions,
// see processing::calculateImageStats.)
struct ImageStats {
    double min = 0.0;
    double max = 0.0;
    double mean = 0.0;
    qint64 finiteCount = 0;     // pixels used for the statistics
    qint64 nonFiniteCount = 0;  // NaN / Inf pixels, left out of the statistics
    bool integerValued = true;  // every finite pixel is a whole number

    bool isValid() const { return finiteCount > 0; }
};

// One 2-D image of doubles, stored row by row.
//
// Pixel (x, y) is column x, row y, both 0-based. Row y corresponds to MATLAB
// row y+1 and column x to MATLAB column x+1, so an image made from a MATLAB
// array A looks the same as imagesc(A).
//
// The pixel buffer is implicitly shared: copying an Image2D is cheap and the
// data is only duplicated when one of the copies is modified.
class Image2D {
public:
    Image2D() = default;
    Image2D(int width, int height, double fill = 0.0);

    int width() const { return m_width; }
    int height() const { return m_height; }
    qsizetype pixelCount() const { return m_data.size(); }
    bool isNull() const { return m_data.isEmpty(); }
    bool sameSize(const Image2D& other) const
    {
        return m_width == other.m_width && m_height == other.m_height;
    }

    double at(int x, int y) const { return m_data[qsizetype(y) * m_width + x]; }
    const double* constData() const { return m_data.constData(); }
    double* data() { return m_data.data(); }

    ImageStats computeStats() const;

private:
    int m_width = 0;
    int m_height = 0;
    QVector<double> m_data;
};

} // namespace iat
