#include "core/Processing.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace iat::processing {

Image2D meanOverFrames(const ImageStack& stack)
{
    if (stack.isNull())
        return {};

    const int rows = stack.rows();
    const int cols = stack.cols();
    const int frames = stack.frameCount();
    const qsizetype plane = qsizetype(rows) * cols;
    const QByteArray& raw = stack.rawData();

    // Accumulate in storage (column-major) order, which reads memory sequentially.
    std::vector<double> sum(std::size_t(plane), 0.0);
    dispatchElementType(stack.elementType(), [&](auto tag) {
        using T = decltype(tag);
        for (int z = 0; z < frames; ++z) {
            const char* p = raw.constData() + qsizetype(sizeof(T)) * plane * z;
            for (qsizetype i = 0; i < plane; ++i)
                sum[std::size_t(i)] += static_cast<double>(loadAs<T>(p + sizeof(T) * i));
        }
    });

    Image2D out(cols, rows);
    double* dst = out.data();
    const double n = double(frames);
    for (int c = 0; c < cols; ++c)
        for (int r = 0; r < rows; ++r)
            dst[qsizetype(r) * cols + c] = sum[std::size_t(qsizetype(c) * rows + r)] / n;
    return out;
}

Image2D subtract(const Image2D& a, const Image2D& b)
{
    if (a.isNull() || !a.sameSize(b))
        return {};

    Image2D out(a.width(), a.height());
    const double* pa = a.constData();
    const double* pb = b.constData();
    double* dst = out.data();
    const qsizetype n = a.pixelCount();
    for (qsizetype i = 0; i < n; ++i)
        dst[i] = pa[i] - pb[i];
    return out;
}

namespace {

// Compensated (Neumaier) summation: keeps sums of millions of pixels
// accurate to the last digits, instead of losing ~1e-10 relative accuracy.
struct CompensatedSum {
    double sum = 0.0;
    double c = 0.0;
    void add(double x)
    {
        const double t = sum + x;
        if (std::abs(sum) >= std::abs(x))
            c += (sum - t) + x;
        else
            c += (x - t) + sum;
        sum = t;
    }
    double value() const { return sum + c; }
};

// mean(x, "all") and std(x, 1, "all") over n values given by valueAt(i).
template <typename ValueAt>
MeanStd meanAndStd(qsizetype n, ValueAt valueAt)
{
    CompensatedSum sum;
    for (qsizetype i = 0; i < n; ++i)
        sum.add(valueAt(i));
    const double mean = sum.value() / double(n);

    CompensatedSum sq;
    for (qsizetype i = 0; i < n; ++i) {
        const double d = valueAt(i) - mean;
        sq.add(d * d);
    }
    return {mean, std::sqrt(sq.value() / double(n))};
}

} // namespace

MeanStd calculateImageStats(const Image2D& image)
{
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    const qsizetype n = image.pixelCount();
    if (n == 0)
        return {nan, nan};

    const double* p = image.constData();
    return meanAndStd(n, [p](qsizetype i) { return p[i]; });
}

MeanStd calculateImageStats(const ImageStack& stack)
{
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    if (stack.isNull())
        return {nan, nan};

    const qsizetype n = qsizetype(stack.rows()) * stack.cols() * stack.frameCount();
    const char* raw = stack.rawData().constData();
    return dispatchElementType(stack.elementType(), [&](auto tag) {
        using T = decltype(tag);
        return meanAndStd(n, [raw](qsizetype i) {
            return static_cast<double>(loadAs<T>(raw + sizeof(T) * i));
        });
    });
}

namespace {

// sqrt(mean(var(stack - offset, 1, 3), "all")), with `offset` given in
// column-major order (or null for no offset). Two passes per pixel: first
// the temporal mean, then the squared deviations from it.
double temporalNoise(const ImageStack& stack, const double* offset)
{
    const qsizetype plane = qsizetype(stack.rows()) * stack.cols();
    const int frames = stack.frameCount();
    const char* raw = stack.rawData().constData();

    std::vector<double> mean(std::size_t(plane), 0.0);
    std::vector<double> sq(std::size_t(plane), 0.0);
    dispatchElementType(stack.elementType(), [&](auto tag) {
        using T = decltype(tag);
        auto value = [&](int z, qsizetype i) {
            const double v = static_cast<double>(loadAs<T>(raw + sizeof(T) * (plane * z + i)));
            return offset ? v - offset[i] : v;
        };
        for (int z = 0; z < frames; ++z)
            for (qsizetype i = 0; i < plane; ++i)
                mean[std::size_t(i)] += value(z, i);
        for (double& m : mean)
            m /= double(frames);
        for (int z = 0; z < frames; ++z)
            for (qsizetype i = 0; i < plane; ++i) {
                const double d = value(z, i) - mean[std::size_t(i)];
                sq[std::size_t(i)] += d * d;
            }
    });

    CompensatedSum sumVar;
    for (double s : sq)
        sumVar.add(s / double(frames));  // var(..., 1, 3): normalised by N
    return std::sqrt(sumVar.value() / double(plane));
}

} // namespace

double calculateImageNoise(const ImageStack& stack)
{
    if (stack.isNull())
        return std::numeric_limits<double>::quiet_NaN();
    return temporalNoise(stack, nullptr);
}

double calculateImageNoise(const ImageStack& stack, const Image2D& offset)
{
    if (stack.isNull() || offset.width() != stack.cols() || offset.height() != stack.rows())
        return std::numeric_limits<double>::quiet_NaN();

    // Offset is row-major; the stack is column-major.
    const std::vector<double> colMajor = toColumnMajor(offset);
    return temporalNoise(stack, colMajor.data());
}

std::vector<double> toColumnMajor(const Image2D& image)
{
    const int rows = image.height();
    const int cols = image.width();
    std::vector<double> out(std::size_t(qsizetype(rows) * cols));
    for (int c = 0; c < cols; ++c)
        for (int r = 0; r < rows; ++r)
            out[std::size_t(qsizetype(c) * rows + r)] = image.at(c, r);
    return out;
}

void frameMinusOffset(const ImageStack& stack, int index, const double* offsetColumnMajor, double* out)
{
    if (stack.isNull() || index < 0 || index >= stack.frameCount())
        return;
    const qsizetype plane = qsizetype(stack.rows()) * stack.cols();
    dispatchElementType(stack.elementType(), [&](auto tag) {
        using T = decltype(tag);
        const char* src = stack.rawData().constData() + qsizetype(sizeof(T)) * plane * index;
        for (qsizetype i = 0; i < plane; ++i) {
            const double v = static_cast<double>(loadAs<T>(src + sizeof(T) * i));
            out[i] = offsetColumnMajor ? v - offsetColumnMajor[i] : v;
        }
    });
}

namespace {

std::vector<double> finiteValues(const Image2D& image)
{
    std::vector<double> values;
    values.reserve(std::size_t(image.pixelCount()));
    const double* p = image.constData();
    for (qsizetype i = 0; i < image.pixelCount(); ++i)
        if (std::isfinite(p[i]))
            values.push_back(p[i]);
    return values;
}

std::size_t rankFor(double fraction, std::size_t count)
{
    fraction = std::clamp(fraction, 0.0, 1.0);
    return std::size_t(std::llround(fraction * double(count - 1)));
}

} // namespace

double percentile(const Image2D& image, double fraction)
{
    std::vector<double> values = finiteValues(image);
    if (values.empty())
        return std::numeric_limits<double>::quiet_NaN();

    const std::size_t k = rankFor(fraction, values.size());
    std::nth_element(values.begin(), values.begin() + std::ptrdiff_t(k), values.end());
    return values[k];
}

std::pair<double, double> percentileRange(const Image2D& image, double lowFraction, double highFraction)
{
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> values = finiteValues(image);
    if (values.empty())
        return {nan, nan};

    std::size_t kLow = rankFor(lowFraction, values.size());
    std::size_t kHigh = rankFor(highFraction, values.size());
    if (kLow > kHigh)
        std::swap(kLow, kHigh);
    // After the first nth_element everything above kHigh is >= values[kHigh],
    // so the second search only needs the part below it.
    std::nth_element(values.begin(), values.begin() + std::ptrdiff_t(kHigh), values.end());
    const double high = values[kHigh];
    std::nth_element(values.begin(), values.begin() + std::ptrdiff_t(kLow), values.begin() + std::ptrdiff_t(kHigh));
    const double low = values[kLow];
    return {low, high};
}

} // namespace iat::processing
