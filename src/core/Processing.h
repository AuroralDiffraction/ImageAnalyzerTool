#pragma once

#include "core/Image2D.h"
#include "core/ImageStack.h"

#include <utility>
#include <vector>

namespace iat::processing {

// Pixel-by-pixel mean over all frames of a stack. MATLAB: mean(stack, 3).
// The result has width = stack.cols(), height = stack.rows().
Image2D meanOverFrames(const ImageStack& stack);

// a - b, pixel by pixel. Returns a null image if the sizes differ.
Image2D subtract(const Image2D& a, const Image2D& b);

// The pixels of `image` in column-major (MATLAB) order.
std::vector<double> toColumnMajor(const Image2D& image);

// Writes stack(:, :, index) - offset to `out` (rows × cols values, column-
// major). `offsetColumnMajor` is rows × cols values in column-major order,
// or null for no offset. Used to export the calibrated stack frame by frame.
void frameMinusOffset(const ImageStack& stack, int index, const double* offsetColumnMajor, double* out);

// The value below which `fraction` (0..1) of the finite pixels lie
// (nearest-rank percentile). Returns NaN for an image with no finite pixels.
double percentile(const Image2D& image, double fraction);

// ---------------------------------------------------------------------------
// Ports of the MATLAB functions in matlab/ (same definitions, same results).
// ---------------------------------------------------------------------------

struct MeanStd {
    double mean = 0.0;
    double std = 0.0;
};

// calculateImageStats.m
//   img_mean = mean(double(input_img), "all");
//   img_std  = std(double(input_img), 1, "all");   % normalised by N
// As in MATLAB, a NaN anywhere in the input makes both results NaN.
// Returns NaN for an empty input.
MeanStd calculateImageStats(const Image2D& image);
MeanStd calculateImageStats(const ImageStack& stack);  // over all frames

// calculateImageNoise.m
//   img_noise = sqrt(mean(var(double(input_img), 1, 3), "all"));
// The temporal variance of every pixel across the Z frames (normalised by
// N), averaged over all pixels, then the square root: the RMS temporal noise
// in DN. A single frame gives 0, as in MATLAB. Returns NaN for an empty stack.
double calculateImageNoise(const ImageStack& stack);

// Same, for the stack with `offset` subtracted from every frame, i.e.
// calculateImageNoise(stack - offset) where offset is rows × cols (for
// example the calibrated stack light - mean(dark, 3)). Returns NaN if the
// sizes differ.
double calculateImageNoise(const ImageStack& stack, const Image2D& offset);

// Two percentiles at once (cheaper than two percentile() calls), e.g.
// percentileRange(img, 0.005, 0.995) for an automatic contrast stretch.
// Returns {NaN, NaN} for an image with no finite pixels.
std::pair<double, double> percentileRange(const Image2D& image, double lowFraction, double highFraction);

} // namespace iat::processing
