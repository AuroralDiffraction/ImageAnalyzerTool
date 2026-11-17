#include "core/Isp.h"

#include <QCoreApplication>
#include <QElapsedTimer>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <thread>

namespace iat::isp {

// ===========================================================================
// Tables: stages, algorithms, parameters
// ===========================================================================

namespace {

ParamSpec real(const char* id, const char* label, double min, double max, double step, double def,
               const char* unit = "")
{
    ParamSpec p;
    p.id = id;
    p.label = label;
    p.kind = ParamKind::Real;
    p.min = min;
    p.max = max;
    p.step = step;
    p.def = def;
    p.unit = unit;
    return p;
}

ParamSpec logReal(const char* id, const char* label, double min, double max, double def, const char* unit = "")
{
    ParamSpec p = real(id, label, min, max, 0.0, def, unit);
    p.logScale = true;
    return p;
}

ParamSpec integer(const char* id, const char* label, int min, int max, int def, const char* unit = "")
{
    ParamSpec p = real(id, label, min, max, 1.0, def, unit);
    p.kind = ParamKind::Integer;
    return p;
}

ParamSpec choice(const char* id, const char* label, std::vector<const char*> choices, int def = 0)
{
    ParamSpec p;
    p.id = id;
    p.label = label;
    p.kind = ParamKind::Choice;
    p.min = 0.0;
    p.max = double(choices.size()) - 1.0;
    p.step = 1.0;
    p.def = def;
    p.choices = std::move(choices);
    return p;
}

AlgorithmSpec algorithm(const char* id, const char* name, const char* description, std::vector<ParamSpec> params = {})
{
    AlgorithmSpec a;
    a.id = id;
    a.name = name;
    a.description = description;
    a.params = std::move(params);
    return a;
}

StageSpec makeStage(StageId id, const char* key, const char* name, const char* shortName, const char* description,
                    std::vector<AlgorithmSpec> algorithms)
{
    StageSpec s;
    s.id = id;
    s.key = key;
    s.name = name;
    s.shortName = shortName;
    s.description = description;
    s.algorithms = std::move(algorithms);
    return s;
}

// Units and labels used by several parameters.
const char* const kPx = QT_TRANSLATE_NOOP("iat::isp", "px");
const char* const kTimesSigma = QT_TRANSLATE_NOOP("iat::isp", "× noise σ");
const char* const kTimes = QT_TRANSLATE_NOOP("iat::isp", "×");
const char* const kStops = QT_TRANSLATE_NOOP("iat::isp", "stops");
const char* const kPercent = "%";
const char* const kDn = "DN";

ParamSpec clipParam()
{
    return choice("clip", QT_TRANSLATE_NOOP("iat::isp", "Negative values"),
                  {QT_TRANSLATE_NOOP("iat::isp", "Clip at 0"), QT_TRANSLATE_NOOP("iat::isp", "Keep")});
}

ParamSpec defectTypeParam()
{
    return choice("type", QT_TRANSLATE_NOOP("iat::isp", "Correct"),
                  {QT_TRANSLATE_NOOP("iat::isp", "Hot and dead pixels"),
                   QT_TRANSLATE_NOOP("iat::isp", "Hot pixels only"),
                   QT_TRANSLATE_NOOP("iat::isp", "Dead pixels only")});
}

ParamSpec directionParam()
{
    return choice("dir", QT_TRANSLATE_NOOP("iat::isp", "Direction"),
                  {QT_TRANSLATE_NOOP("iat::isp", "Rows and columns"), QT_TRANSLATE_NOOP("iat::isp", "Rows"),
                   QT_TRANSLATE_NOOP("iat::isp", "Columns")});
}

ParamSpec outputDepthParam()
{
    return choice("bits", QT_TRANSLATE_NOOP("iat::isp", "Output depth"),
                  {QT_TRANSLATE_NOOP("iat::isp", "8-bit"), QT_TRANSLATE_NOOP("iat::isp", "6-bit"),
                   QT_TRANSLATE_NOOP("iat::isp", "4-bit")});
}

std::vector<StageSpec> buildStages()
{
    std::vector<StageSpec> s;

    s.push_back(makeStage(
        StageId::BlackLevel, "black_level", QT_TRANSLATE_NOOP("iat::isp", "Black level"),
        QT_TRANSLATE_NOOP("iat::isp", "Black level"),
        QT_TRANSLATE_NOOP("iat::isp", "Subtracts the sensor's pedestal, so that no light reads as zero."),
        {algorithm("fixed", QT_TRANSLATE_NOOP("iat::isp", "Fixed value"),
                   QT_TRANSLATE_NOOP("iat::isp", "Subtracts a constant: the black level from the sensor's data "
                                                 "sheet, or the mean of your dark frames."),
                   {real("offset", QT_TRANSLATE_NOOP("iat::isp", "Black level"), 0, 4096, 1, 64, kDn), clipParam()}),
         algorithm("auto", QT_TRANSLATE_NOOP("iat::isp", "Automatic (low percentile)"),
                   QT_TRANSLATE_NOOP("iat::isp", "Uses a low percentile of the image as the black level. Only "
                                                 "right when part of the image is truly dark."),
                   {logReal("pct", QT_TRANSLATE_NOOP("iat::isp", "Percentile"), 0.01, 10, 0.1, kPercent),
                    clipParam()})}));

    s.push_back(makeStage(
        StageId::DefectPixel, "defect_pixel", QT_TRANSLATE_NOOP("iat::isp", "Defect pixels"),
        QT_TRANSLATE_NOOP("iat::isp", "Defect pixels"),
        QT_TRANSLATE_NOOP("iat::isp", "Replaces hot, dead and stuck pixels with a value from their neighbours."),
        {algorithm("minmax", QT_TRANSLATE_NOOP("iat::isp", "Neighbour min / max"),
                   QT_TRANSLATE_NOOP("iat::isp", "A pixel brighter than its brightest neighbour, or darker than "
                                                 "its darkest, by more than the threshold is replaced by the "
                                                 "median of the 8 neighbours. Leaves edges and thin lines alone. "
                                                 "In textured areas the threshold rises with the local contrast."),
                   {logReal("k", QT_TRANSLATE_NOOP("iat::isp", "Threshold"), 2, 100, 5, kTimesSigma),
                    defectTypeParam()}),
         algorithm("median", QT_TRANSLATE_NOOP("iat::isp", "Median threshold"),
                   QT_TRANSLATE_NOOP("iat::isp", "A pixel that differs from the median of its 8 neighbours by more "
                                                 "than the threshold is replaced by that median. Also catches "
                                                 "pairs of defects, but can remove single-pixel detail such as "
                                                 "stars. In textured areas the threshold rises with the local "
                                                 "contrast."),
                   {logReal("k", QT_TRANSLATE_NOOP("iat::isp", "Threshold"), 2, 100, 8, kTimesSigma),
                    defectTypeParam()})}));

    s.push_back(makeStage(
        StageId::DefectLine, "defect_line", QT_TRANSLATE_NOOP("iat::isp", "Defect rows / columns"),
        QT_TRANSLATE_NOOP("iat::isp", "Defect lines"),
        QT_TRANSLATE_NOOP("iat::isp", "Repairs whole rows or columns that are dead, hot or offset from their "
                                      "neighbours."),
        {algorithm("replace", QT_TRANSLATE_NOOP("iat::isp", "Replace defective lines"),
                   QT_TRANSLATE_NOOP("iat::isp", "A row or column that stands out from the lines on both sides of it "
                                                 "(by more than the threshold, and by more than the pixel noise) is "
                                                 "rebuilt by interpolating between the nearest good lines. Edges in "
                                                 "the picture are left alone."),
                   {directionParam(),
                    logReal("k", QT_TRANSLATE_NOOP("iat::isp", "Threshold"), 3, 50, 6,
                            QT_TRANSLATE_NOOP("iat::isp", "× spread"))}),
         algorithm("destripe", QT_TRANSLATE_NOOP("iat::isp", "Remove line offsets (destripe)"),
                   QT_TRANSLATE_NOOP("iat::isp", "Subtracts from every row or column its offset from a running "
                                                 "median of the lines around it. Removes banding and row / column "
                                                 "fixed-pattern noise."),
                   {directionParam(),
                    integer("window", QT_TRANSLATE_NOOP("iat::isp", "Window"), 3, 101, 15,
                            QT_TRANSLATE_NOOP("iat::isp", "lines")),
                    real("strength", QT_TRANSLATE_NOOP("iat::isp", "Strength"), 0, 1, 0.05, 1)})}));

    s.push_back(makeStage(
        StageId::Gain, "gain", QT_TRANSLATE_NOOP("iat::isp", "Digital gain"),
        QT_TRANSLATE_NOOP("iat::isp", "Gain"),
        QT_TRANSLATE_NOOP("iat::isp", "Multiplies every pixel by the same factor."),
        {algorithm("manual", QT_TRANSLATE_NOOP("iat::isp", "Manual"),
                   QT_TRANSLATE_NOOP("iat::isp", "Multiplies by a fixed gain."),
                   {logReal("gain", QT_TRANSLATE_NOOP("iat::isp", "Gain"), 0.1, 64, 1, kTimes)}),
         algorithm("auto", QT_TRANSLATE_NOOP("iat::isp", "Automatic (mean to target)"),
                   QT_TRANSLATE_NOOP("iat::isp", "Chooses the gain that brings the image mean to a target value, "
                                                 "as auto exposure does."),
                   {logReal("target", QT_TRANSLATE_NOOP("iat::isp", "Target mean"), 1, 65535, 1000, kDn)})}));

    s.push_back(makeStage(
        StageId::Denoise, "denoise", QT_TRANSLATE_NOOP("iat::isp", "Denoise"),
        QT_TRANSLATE_NOOP("iat::isp", "Denoise"),
        QT_TRANSLATE_NOOP("iat::isp", "Reduces pixel noise. Strengths are relative to the noise level estimated "
                                      "from the image."),
        {algorithm("bilateral", QT_TRANSLATE_NOOP("iat::isp", "Bilateral filter"),
                   QT_TRANSLATE_NOOP("iat::isp", "Averages only neighbours of similar value, so noise is smoothed "
                                                 "while edges stay sharp."),
                   {real("sigma_s", QT_TRANSLATE_NOOP("iat::isp", "Spatial radius (σ)"), 0.5, 6, 0.1, 2, kPx),
                    logReal("k", QT_TRANSLATE_NOOP("iat::isp", "Range"), 0.5, 20, 3, kTimesSigma)}),
         algorithm("guided", QT_TRANSLATE_NOOP("iat::isp", "Guided filter"),
                   QT_TRANSLATE_NOOP("iat::isp", "Edge-preserving smoothing from local linear models. Fast at any "
                                                 "radius, without the bilateral filter's gradient artefacts."),
                   {integer("radius", QT_TRANSLATE_NOOP("iat::isp", "Radius"), 1, 16, 4, kPx),
                    logReal("k", QT_TRANSLATE_NOOP("iat::isp", "Edge threshold"), 0.5, 20, 2, kTimesSigma)}),
         algorithm("nlm", QT_TRANSLATE_NOOP("iat::isp", "Non-local means"),
                   QT_TRANSLATE_NOOP("iat::isp", "Averages pixels whose surrounding patches look alike, anywhere in "
                                                 "a search window. Keeps texture best; the slowest choice."),
                   {integer("search", QT_TRANSLATE_NOOP("iat::isp", "Search radius"), 2, 10, 4, kPx),
                    choice("patch", QT_TRANSLATE_NOOP("iat::isp", "Patch"), {"3 × 3", "5 × 5"}),
                    logReal("k", QT_TRANSLATE_NOOP("iat::isp", "Strength"), 0.2, 3, 0.8, kTimesSigma)}),
         algorithm("median", QT_TRANSLATE_NOOP("iat::isp", "Median filter"),
                   QT_TRANSLATE_NOOP("iat::isp", "Each pixel becomes the median of its neighbourhood. Best for "
                                                 "impulse (salt-and-pepper) noise; keeps edges but rounds corners."),
                   {choice("size", QT_TRANSLATE_NOOP("iat::isp", "Window"), {"3 × 3", "5 × 5"})}),
         algorithm("gaussian", QT_TRANSLATE_NOOP("iat::isp", "Gaussian blur"),
                   QT_TRANSLATE_NOOP("iat::isp", "Weighted average of the neighbourhood. Fast, but blurs edges as "
                                                 "much as noise."),
                   {real("sigma", QT_TRANSLATE_NOOP("iat::isp", "Radius (σ)"), 0.3, 5, 0.1, 1, kPx)})}));

    s.push_back(makeStage(
        StageId::DynamicRange, "dynamic_range", QT_TRANSLATE_NOOP("iat::isp", "Dynamic range compression"),
        QT_TRANSLATE_NOOP("iat::isp", "Dynamic range"),
        QT_TRANSLATE_NOOP("iat::isp", "Reduces the contrast between bright and dark regions so that both fit the "
                                      "display. The result stays in DN, ready for tone mapping."),
        {algorithm("local", QT_TRANSLATE_NOOP("iat::isp", "Local (base / detail)"),
                   QT_TRANSLATE_NOOP("iat::isp", "In log space an edge-preserving filter separates a smooth base "
                                                 "from the detail. Only the base is compressed, so textures keep "
                                                 "their contrast."),
                   {real("range", QT_TRANSLATE_NOOP("iat::isp", "Base contrast"), 2, 14, 0.1, 6, kStops),
                    integer("radius", QT_TRANSLATE_NOOP("iat::isp", "Filter radius"), 4, 128, 24, kPx),
                    logReal("eps", QT_TRANSLATE_NOOP("iat::isp", "Edge threshold"), 0.02, 4, 0.4)}),
         algorithm("log", QT_TRANSLATE_NOOP("iat::isp", "Global log curve"),
                   QT_TRANSLATE_NOOP("iat::isp", "out = log(1 + a·x) / log(1 + a): equal ratios of input become "
                                                 "roughly equal steps of output."),
                   {logReal("a", QT_TRANSLATE_NOOP("iat::isp", "Strength"), 3, 10000, 300)}),
         algorithm("gamma", QT_TRANSLATE_NOOP("iat::isp", "Global gamma curve"),
                   QT_TRANSLATE_NOOP("iat::isp", "Raises the normalised image to the power 1/γ, which lifts the "
                                                 "shadows."),
                   {real("g", QT_TRANSLATE_NOOP("iat::isp", "Gamma"), 1, 4, 0.05, 2.2)})}));

    s.push_back(makeStage(
        StageId::ToneMap, "tone_map", QT_TRANSLATE_NOOP("iat::isp", "Tone mapping"),
        QT_TRANSLATE_NOOP("iat::isp", "Tone mapping"),
        QT_TRANSLATE_NOOP("iat::isp", "Maps the full range of the data to the 256 grey levels of a display. The "
                                      "result is in display values, 0 to 255."),
        {algorithm("stretch", QT_TRANSLATE_NOOP("iat::isp", "Percentile stretch"),
                   QT_TRANSLATE_NOOP("iat::isp", "Stretches the window between two percentiles of the histogram "
                                                 "linearly to 0–255 (auto levels). Good for low-contrast data on a "
                                                 "pedestal; everything outside the window clips."),
                   {real("lo", QT_TRANSLATE_NOOP("iat::isp", "Clip darkest"), 0, 5, 0.05, 0.5, kPercent),
                    real("hi", QT_TRANSLATE_NOOP("iat::isp", "Clip brightest"), 0, 5, 0.05, 0.5, kPercent)}),
         algorithm("gamma", QT_TRANSLATE_NOOP("iat::isp", "Gamma curve"),
                   QT_TRANSLATE_NOOP("iat::isp", "Normalises to the robust min–max and raises to the power 1/γ. "
                                                 "γ = 2.2 is roughly what a camera does for an sRGB image. Good for "
                                                 "ordinary scenes of moderate range."),
                   {real("g", QT_TRANSLATE_NOOP("iat::isp", "Gamma"), 1, 4, 0.05, 2.2)}),
         algorithm("log", QT_TRANSLATE_NOOP("iat::isp", "Log curve"),
                   QT_TRANSLATE_NOOP("iat::isp", "out = log(1 + a·x) / log(1 + a). Every stop gets some codes; "
                                                 "midtones look flat and shadow noise is amplified the most."),
                   {logReal("a", QT_TRANSLATE_NOOP("iat::isp", "Strength"), 3, 10000, 300)}),
         algorithm("reinhard", QT_TRANSLATE_NOOP("iat::isp", "Reinhard global"),
                   QT_TRANSLATE_NOOP("iat::isp", "Scales the log-average of the image to a key value, compresses "
                                                 "with L·(1 + L/L_white²)/(1 + L) and applies display gamma. A "
                                                 "photographic look with a smooth highlight roll-off."),
                   {real("key", QT_TRANSLATE_NOOP("iat::isp", "Key"), 0.04, 0.72, 0.01, 0.18),
                    real("white", QT_TRANSLATE_NOOP("iat::isp", "White point"), 0.1, 1, 0.01, 1,
                         QT_TRANSLATE_NOOP("iat::isp", "× max"))}),
         algorithm("he", QT_TRANSLATE_NOOP("iat::isp", "Plateau equalisation"),
                   QT_TRANSLATE_NOOP("iat::isp", "Histogram equalisation with every bin capped at a plateau, so "
                                                 "large flat areas cannot take all the codes. Output codes follow "
                                                 "the pixel population: the usual automatic gain of thermal cameras."),
                   {logReal("plateau", QT_TRANSLATE_NOOP("iat::isp", "Plateau limit"), 0.05, 200, 1,
                            QT_TRANSLATE_NOOP("iat::isp", "× mean"))}),
         algorithm("clahe", QT_TRANSLATE_NOOP("iat::isp", "CLAHE"),
                   QT_TRANSLATE_NOOP("iat::isp", "Equalises each tile with a clipped histogram and blends the "
                                                 "neighbouring tile curves. Shows detail everywhere at once; "
                                                 "brightness is no longer comparable across the frame."),
                   {integer("tiles", QT_TRANSLATE_NOOP("iat::isp", "Tiles per side"), 2, 16, 8),
                    real("clip", QT_TRANSLATE_NOOP("iat::isp", "Clip limit"), 1, 8, 0.1, 2.5)}),
         algorithm("local", QT_TRANSLATE_NOOP("iat::isp", "Base / detail split"),
                   QT_TRANSLATE_NOOP("iat::isp", "In log space a guided filter splits the image into a smooth base "
                                                 "and a detail layer. Only the base is compressed; detail is added "
                                                 "back with gain. The usual choice for real high dynamic range."),
                   {real("range", QT_TRANSLATE_NOOP("iat::isp", "Base contrast"), 2, 10, 0.1, 4.5, kStops),
                    real("detail", QT_TRANSLATE_NOOP("iat::isp", "Detail gain"), 0.5, 3, 0.05, 1.3, kTimes),
                    integer("radius", QT_TRANSLATE_NOOP("iat::isp", "Filter radius"), 4, 128, 24, kPx),
                    logReal("eps", QT_TRANSLATE_NOOP("iat::isp", "Edge threshold"), 0.02, 4, 0.4)}),
         algorithm("fusion", QT_TRANSLATE_NOOP("iat::isp", "Exposure fusion"),
                   QT_TRANSLATE_NOOP("iat::isp", "Renders several virtual exposures from the one frame, weights "
                                                 "each pixel by how close it is to mid-grey and blends them with "
                                                 "Laplacian pyramids (Mertens). Natural and halo-resistant."),
                   {integer("k", QT_TRANSLATE_NOOP("iat::isp", "Exposures"), 2, 6, 4),
                    real("span", QT_TRANSLATE_NOOP("iat::isp", "Exposure span"), 1, 10, 0.25, 6, kStops),
                    real("sigma", QT_TRANSLATE_NOOP("iat::isp", "Selectivity"), 0.1, 0.45, 0.01, 0.2)}),
         algorithm("shift", QT_TRANSLATE_NOOP("iat::isp", "Bit shift"),
                   QT_TRANSLATE_NOOP("iat::isp", "Keeps the top 8 bits, so full scale maps linearly to 0–255. A "
                                                 "reference point: nothing clips, but shadows collapse to a few "
                                                 "codes."),
                   {choice("bits", QT_TRANSLATE_NOOP("iat::isp", "Input depth"),
                           {QT_TRANSLATE_NOOP("iat::isp", "Automatic"), QT_TRANSLATE_NOOP("iat::isp", "8-bit"),
                            QT_TRANSLATE_NOOP("iat::isp", "10-bit"), QT_TRANSLATE_NOOP("iat::isp", "12-bit"),
                            QT_TRANSLATE_NOOP("iat::isp", "14-bit"), QT_TRANSLATE_NOOP("iat::isp", "16-bit")})})}));

    s.push_back(makeStage(
        StageId::Detail, "detail", QT_TRANSLATE_NOOP("iat::isp", "Detail enhancement"),
        QT_TRANSLATE_NOOP("iat::isp", "Detail"),
        QT_TRANSLATE_NOOP("iat::isp", "Amplifies fine local contrast (texture) without changing the large-scale "
                                      "brightness."),
        {algorithm("guided", QT_TRANSLATE_NOOP("iat::isp", "Base / detail boost"),
                   QT_TRANSLATE_NOOP("iat::isp", "A guided filter splits off the detail layer, which is multiplied "
                                                 "by the gain. Detail below the noise threshold is left as it is, "
                                                 "so noise is not amplified. Edges and slopes stronger than the "
                                                 "edge threshold count as structure, not detail."),
                   {integer("radius", QT_TRANSLATE_NOOP("iat::isp", "Radius"), 2, 32, 8, kPx),
                    real("gain", QT_TRANSLATE_NOOP("iat::isp", "Detail gain"), 0, 5, 0.05, 2, kTimes),
                    logReal("k", QT_TRANSLATE_NOOP("iat::isp", "Edge threshold"), 1, 100, 10, kTimesSigma),
                    real("core", QT_TRANSLATE_NOOP("iat::isp", "Noise threshold"), 0, 5, 0.1, 1, kTimesSigma)}),
         algorithm("multiscale", QT_TRANSLATE_NOOP("iat::isp", "Multi-scale"),
                   QT_TRANSLATE_NOOP("iat::isp", "Splits the image into fine, medium and coarse detail bands "
                                                 "(differences of Gaussians), with a gain for each."),
                   {real("fine", QT_TRANSLATE_NOOP("iat::isp", "Fine (1 px)"), 0, 4, 0.05, 1.5, kTimes),
                    real("medium", QT_TRANSLATE_NOOP("iat::isp", "Medium (4 px)"), 0, 4, 0.05, 1.3, kTimes),
                    real("coarse", QT_TRANSLATE_NOOP("iat::isp", "Coarse (16 px)"), 0, 4, 0.05, 1, kTimes)})}));

    s.push_back(makeStage(
        StageId::Sharpen, "sharpen", QT_TRANSLATE_NOOP("iat::isp", "Sharpening"),
        QT_TRANSLATE_NOOP("iat::isp", "Sharpen"),
        QT_TRANSLATE_NOOP("iat::isp", "Steepens edges so the image looks crisper."),
        {algorithm("usm", QT_TRANSLATE_NOOP("iat::isp", "Unsharp mask"),
                   QT_TRANSLATE_NOOP("iat::isp", "Adds back the difference between the image and a blurred copy. "
                                                 "The threshold skips differences at noise level; limiting the "
                                                 "overshoot prevents bright and dark halos along edges."),
                   {real("sigma", QT_TRANSLATE_NOOP("iat::isp", "Radius (σ)"), 0.3, 5, 0.1, 1, kPx),
                    real("amount", QT_TRANSLATE_NOOP("iat::isp", "Amount"), 0, 5, 0.05, 1, kTimes),
                    real("thr", QT_TRANSLATE_NOOP("iat::isp", "Threshold"), 0, 10, 0.1, 1, kTimesSigma),
                    choice("limit", QT_TRANSLATE_NOOP("iat::isp", "Overshoot"),
                           {QT_TRANSLATE_NOOP("iat::isp", "Allow"),
                            QT_TRANSLATE_NOOP("iat::isp", "Limit to local min / max")},
                           1)}),
         algorithm("laplace", QT_TRANSLATE_NOOP("iat::isp", "Laplacian"),
                   QT_TRANSLATE_NOOP("iat::isp", "Subtracts the Laplacian (second derivative). The simplest "
                                                 "sharpener: it acts on the finest scale only and amplifies noise."),
                   {real("amount", QT_TRANSLATE_NOOP("iat::isp", "Amount"), 0, 3, 0.05, 0.5, kTimes)})}));

    s.push_back(makeStage(
        StageId::Output, "output", QT_TRANSLATE_NOOP("iat::isp", "Output quantisation"),
        QT_TRANSLATE_NOOP("iat::isp", "Quantise"),
        QT_TRANSLATE_NOOP("iat::isp", "Rounds the display values to whole grey levels. It needs tone mapping. Lower "
                                      "depths exaggerate banding, so you can see what dither does."),
        {algorithm("round", QT_TRANSLATE_NOOP("iat::isp", "Round"),
                   QT_TRANSLATE_NOOP("iat::isp", "Rounds to the nearest level. Smooth gradients can show bands."),
                   {outputDepthParam()}),
         algorithm("noise", QT_TRANSLATE_NOOP("iat::isp", "Random dither (TPDF)"),
                   QT_TRANSLATE_NOOP("iat::isp", "Adds triangular random noise of ±1 level before rounding: bands "
                                                 "turn into fine grain."),
                   {outputDepthParam()}),
         algorithm("bayer", QT_TRANSLATE_NOOP("iat::isp", "Ordered dither (Bayer 8 × 8)"),
                   QT_TRANSLATE_NOOP("iat::isp", "Adds a fixed 8 × 8 threshold pattern before rounding: a regular "
                                                 "texture instead of bands."),
                   {outputDepthParam()}),
         algorithm("fs", QT_TRANSLATE_NOOP("iat::isp", "Floyd–Steinberg"),
                   QT_TRANSLATE_NOOP("iat::isp", "Passes each pixel's rounding error on to its neighbours (error "
                                                 "diffusion)."),
                   {outputDepthParam()})}));

    return s;
}

} // namespace

const std::vector<StageSpec>& stages()
{
    static const std::vector<StageSpec> s = buildStages();
    return s;
}

int stageIndex(StageId id) { return int(id); }

const StageSpec& stage(StageId id) { return stages()[std::size_t(stageIndex(id))]; }

QString text(const char* source)
{
    if (!source || !*source)
        return {};
    return QCoreApplication::translate("iat::isp", source);
}

// ===========================================================================
// Pipeline (settings)
// ===========================================================================

namespace {

StageConfig defaultConfig(const StageSpec& spec)
{
    StageConfig c;
    for (const AlgorithmSpec& a : spec.algorithms) {
        QVector<double> v;
        for (const ParamSpec& p : a.params)
            v.append(p.def);
        c.values.append(v);
    }
    return c;
}

int paramIndex(const AlgorithmSpec& a, const char* paramId)
{
    for (std::size_t i = 0; i < a.params.size(); ++i)
        if (std::strcmp(a.params[i].id, paramId) == 0)
            return int(i);
    return -1;
}

} // namespace

Pipeline::Pipeline()
{
    for (const StageSpec& s : stages())
        m_stages.append(defaultConfig(s));
}

bool Pipeline::isEnabled(StageId id) const
{
    return config(id).enabled;
}

void Pipeline::setEnabled(StageId id, bool on)
{
    config(id).enabled = on;
}

bool Pipeline::setAlgorithm(StageId id, const char* algorithmId)
{
    const StageSpec& spec = stage(id);
    for (std::size_t i = 0; i < spec.algorithms.size(); ++i) {
        if (std::strcmp(spec.algorithms[i].id, algorithmId) == 0) {
            config(id).algorithm = int(i);
            return true;
        }
    }
    return false;
}

const AlgorithmSpec* Pipeline::algorithm(StageId id) const
{
    const StageSpec& spec = stage(id);
    const int a = config(id).algorithm;
    if (a < 0 || a >= int(spec.algorithms.size()))
        return nullptr;
    return &spec.algorithms[std::size_t(a)];
}

double Pipeline::value(StageId id, const char* paramId) const
{
    const AlgorithmSpec* a = algorithm(id);
    const int i = a ? paramIndex(*a, paramId) : -1;
    if (i < 0)
        return std::numeric_limits<double>::quiet_NaN();
    return config(id).values[config(id).algorithm][i];
}

bool Pipeline::setValue(StageId id, const char* paramId, double value)
{
    const AlgorithmSpec* a = algorithm(id);
    const int i = a ? paramIndex(*a, paramId) : -1;
    if (i < 0 || std::isnan(value))
        return false;
    const ParamSpec& p = a->params[std::size_t(i)];
    value = std::clamp(value, p.min, p.max);
    if (p.kind != ParamKind::Real)
        value = std::round(value);
    config(id).values[config(id).algorithm][i] = value;
    return true;
}

void Pipeline::resetStage(StageId id) { config(id) = defaultConfig(stage(id)); }

bool Pipeline::isActive() const
{
    for (const StageSpec& s : stages())
        if (isEnabled(s.id))
            return true;
    return false;
}

bool Pipeline::sameScaleAs(const Pipeline& other) const
{
    // The stages that change what the pixel values mean.
    for (StageId id : {StageId::BlackLevel, StageId::Gain, StageId::DynamicRange, StageId::ToneMap}) {
        if (isEnabled(id) != other.isEnabled(id))
            return false;
        if (isEnabled(id) && config(id) != other.config(id))
            return false;
    }
    return true;
}

QString Pipeline::summary(bool translated) const
{
    QStringList parts;
    for (const StageSpec& s : stages()) {
        if (!isEnabled(s.id))
            continue;
        if (s.id == StageId::Output && !isEnabled(StageId::ToneMap))
            continue;  // not applied without tone mapping
        const AlgorithmSpec* a = algorithm(s.id);
        const QString stageName = translated ? text(s.name) : QString::fromUtf8(s.name);
        const QString algName = !a ? QString() : translated ? text(a->name) : QString::fromUtf8(a->name);
        parts << (algName.isEmpty() ? stageName : stageName + QStringLiteral(": ") + algName);
    }
    return parts.join(QStringLiteral("; "));
}

// ===========================================================================
// Image buffer and filters
// ===========================================================================

namespace {

struct Buffer {
    int w = 0;
    int h = 0;
    std::vector<double> d;

    std::size_t n() const { return d.size(); }
    double at(int x, int y) const { return d[std::size_t(y) * w + x]; }
};

inline double clamp01(double v) { return v < 0.0 ? 0.0 : v > 1.0 ? 1.0 : v; }
inline int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

// Runs fn(y0, y1) over [0, rows) on several threads.
void parallelRows(int rows, const std::function<void(int, int)>& fn)
{
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    const int threads = int(std::min<unsigned>(hw, 16u));
    if (threads <= 1 || rows < 64) {
        fn(0, rows);
        return;
    }
    std::vector<std::thread> pool;
    const int chunk = (rows + threads - 1) / threads;
    for (int y0 = chunk; y0 < rows; y0 += chunk)
        pool.emplace_back(fn, y0, std::min(rows, y0 + chunk));
    fn(0, std::min(rows, chunk));
    for (std::thread& t : pool)
        t.join();
}

// The value at position ceil(q·n) in sorted order (1-based), as the
// histogram percentile of the reference implementation. Reorders `v`.
double quantileInPlace(std::vector<double>& v, double q)
{
    if (v.empty())
        return 0.0;
    const double t = std::max(1.0, q * double(v.size()));
    const std::size_t k = std::min(v.size() - 1, std::size_t(std::ceil(t)) - 1);
    std::nth_element(v.begin(), v.begin() + std::ptrdiff_t(k), v.end());
    return v[k];
}

double medianInPlace(std::vector<double>& v)
{
    if (v.empty())
        return 0.0;
    const std::size_t k = v.size() / 2;
    std::nth_element(v.begin(), v.begin() + std::ptrdiff_t(k), v.end());
    return v[k];
}

struct Range {
    double lo = 0.0;
    double hi = 1.0;
};

Range quantileRange(const Buffer& b, double qlo, double qhi)
{
    std::vector<double> v = b.d;
    Range r;
    r.lo = quantileInPlace(v, qlo);
    r.hi = quantileInPlace(v, qhi);
    return r;
}

// The 0.01 % and 99.99 % percentiles: min and max without a few outliers.
Range robustRange(const Buffer& b)
{
    Range r = quantileRange(b, 1e-4, 1.0 - 1e-4);
    if (!(r.hi > r.lo)) {
        const auto [mn, mx] = std::minmax_element(b.d.begin(), b.d.end());
        r.lo = *mn;
        r.hi = *mx;
    }
    if (!(r.hi > r.lo))
        r.hi = r.lo + 1.0;
    return r;
}

std::vector<double> normalised(const Buffer& b, const Range& r)
{
    std::vector<double> v(b.n());
    const double s = 1.0 / (r.hi - r.lo);
    for (std::size_t i = 0; i < v.size(); ++i)
        v[i] = clamp01((b.d[i] - r.lo) * s);
    return v;
}

double noiseSigma(const Buffer& b)
{
    // |I * [1 -2 1; -2 4 -2; 1 -2 1]| has standard deviation 6σ for
    // independent noise and is blind to constant and linear image content.
    const int step = std::max(1, int(std::sqrt(double(b.n()) / 250000.0)));
    std::vector<double> v;
    v.reserve(std::size_t((b.w / step + 1)) * std::size_t(b.h / step + 1));
    for (int y = 1; y < b.h - 1; y += step) {
        const double* r0 = &b.d[std::size_t(y - 1) * b.w];
        const double* r1 = r0 + b.w;
        const double* r2 = r1 + b.w;
        for (int x = 1; x < b.w - 1; x += step) {
            const double l = r0[x - 1] - 2 * r0[x] + r0[x + 1] - 2 * r1[x - 1] + 4 * r1[x] - 2 * r1[x + 1]
                             + r2[x - 1] - 2 * r2[x] + r2[x + 1];
            v.push_back(std::abs(l));
        }
    }
    const double sigma = medianInPlace(v) / (0.6745 * 6.0);
    // Never zero, so thresholds "× noise σ" stay meaningful on noise-free
    // or coarsely quantised images.
    const Range r = quantileRange(b, 0.001, 0.999);
    return std::max({sigma, 5e-5 * (r.hi - r.lo), 1e-12});
}

// Box filter of radius r (the window shrinks at the borders).
std::vector<double> boxBlur(const std::vector<double>& src, int w, int h, int r)
{
    std::vector<double> tmp(src.size()), dst(src.size());
    parallelRows(h, [&](int y0, int y1) {
        std::vector<double> pre(std::size_t(w) + 1);
        for (int y = y0; y < y1; ++y) {
            const std::size_t o = std::size_t(y) * w;
            pre[0] = 0.0;
            for (int x = 0; x < w; ++x)
                pre[std::size_t(x) + 1] = pre[std::size_t(x)] + src[o + x];
            for (int x = 0; x < w; ++x) {
                const int a = std::max(0, x - r), b = std::min(w - 1, x + r);
                tmp[o + x] = (pre[std::size_t(b) + 1] - pre[std::size_t(a)]) / double(b - a + 1);
            }
        }
    });
    // Columns: prefix sums down every column, built row by row (cache friendly).
    std::vector<double> pre((std::size_t(h) + 1) * w, 0.0);
    for (int y = 0; y < h; ++y) {
        const double* row = &tmp[std::size_t(y) * w];
        const double* p0 = &pre[std::size_t(y) * w];
        double* p1 = &pre[std::size_t(y + 1) * w];
        for (int x = 0; x < w; ++x)
            p1[x] = p0[x] + row[x];
    }
    parallelRows(h, [&](int y0, int y1) {
        for (int y = y0; y < y1; ++y) {
            const int a = std::max(0, y - r), b = std::min(h - 1, y + r);
            const double inv = 1.0 / double(b - a + 1);
            const double* pa = &pre[std::size_t(a) * w];
            const double* pb = &pre[std::size_t(b + 1) * w];
            double* out = &dst[std::size_t(y) * w];
            for (int x = 0; x < w; ++x)
                out[x] = (pb[x] - pa[x]) * inv;
        }
    });
    return dst;
}

// Gaussian filter, separable, borders replicated.
std::vector<double> gaussianBlur(const std::vector<double>& src, int w, int h, double sigma)
{
    if (!(sigma > 0.05))
        return src;
    const int r = std::max(1, int(std::ceil(3.0 * sigma)));
    std::vector<double> k(std::size_t(2 * r + 1));
    double ks = 0.0;
    for (int i = -r; i <= r; ++i)
        ks += k[std::size_t(i + r)] = std::exp(-0.5 * i * i / (sigma * sigma));
    for (double& v : k)
        v /= ks;

    std::vector<double> tmp(src.size()), dst(src.size());
    parallelRows(h, [&](int y0, int y1) {
        for (int y = y0; y < y1; ++y) {
            const double* row = &src[std::size_t(y) * w];
            double* out = &tmp[std::size_t(y) * w];
            for (int x = 0; x < w; ++x) {
                double acc = 0.0;
                if (x >= r && x < w - r) {
                    for (int i = -r; i <= r; ++i)
                        acc += k[std::size_t(i + r)] * row[x + i];
                } else {
                    for (int i = -r; i <= r; ++i)
                        acc += k[std::size_t(i + r)] * row[clampi(x + i, 0, w - 1)];
                }
                out[x] = acc;
            }
        }
    });
    parallelRows(h, [&](int y0, int y1) {
        for (int y = y0; y < y1; ++y) {
            double* out = &dst[std::size_t(y) * w];
            std::fill(out, out + w, 0.0);
            for (int i = -r; i <= r; ++i) {
                const double* row = &tmp[std::size_t(clampi(y + i, 0, h - 1)) * w];
                const double kv = k[std::size_t(i + r)];
                for (int x = 0; x < w; ++x)
                    out[x] += kv * row[x];
            }
        }
    });
    return dst;
}

// Guided filter with the image as its own guide (He et al.): edge-preserving
// smoothing. eps is a variance: edges much stronger than sqrt(eps) are kept.
std::vector<double> guidedSelf(const std::vector<double>& I, int w, int h, int r, double eps)
{
    const std::size_t n = I.size();
    std::vector<double> II(n);
    for (std::size_t i = 0; i < n; ++i)
        II[i] = I[i] * I[i];
    const std::vector<double> mI = boxBlur(I, w, h, r), mII = boxBlur(II, w, h, r);
    std::vector<double> a(n), b(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double v = std::max(0.0, mII[i] - mI[i] * mI[i]);
        a[i] = v / (v + eps);
        b[i] = mI[i] * (1.0 - a[i]);
    }
    const std::vector<double> ma = boxBlur(a, w, h, r), mb = boxBlur(b, w, h, r);
    std::vector<double> q(n);
    for (std::size_t i = 0; i < n; ++i)
        q[i] = ma[i] * I[i] + mb[i];
    return q;
}

// Median filter with a (2r+1)² window, borders replicated.
std::vector<double> medianFilter(const std::vector<double>& src, int w, int h, int r)
{
    std::vector<double> dst(src.size());
    const int side = 2 * r + 1;
    parallelRows(h, [&](int y0, int y1) {
        std::vector<double> win(std::size_t(side * side));
        for (int y = y0; y < y1; ++y) {
            for (int x = 0; x < w; ++x) {
                std::size_t k = 0;
                for (int j = -r; j <= r; ++j) {
                    const double* row = &src[std::size_t(clampi(y + j, 0, h - 1)) * w];
                    for (int i = -r; i <= r; ++i)
                        win[k++] = row[clampi(x + i, 0, w - 1)];
                }
                std::nth_element(win.begin(), win.begin() + std::ptrdiff_t(k / 2), win.end());
                dst[std::size_t(y) * w + x] = win[k / 2];
            }
        }
    });
    return dst;
}

// exp(-t) for t in [0, 10], tabulated.
struct ExpTable {
    static constexpr int N = 4096;
    static constexpr double Max = 10.0;
    std::array<double, N + 1> v;
    ExpTable()
    {
        for (int i = 0; i <= N; ++i)
            v[std::size_t(i)] = std::exp(-Max * i / N);
    }
    double operator()(double t) const
    {
        if (!(t < Max))
            return 0.0;
        return v[std::size_t(t * (N / Max) + 0.5)];
    }
};
const ExpTable& expTable()
{
    static const ExpTable t;
    return t;
}

std::vector<double> bilateral(const std::vector<double>& src, int w, int h, double sigmaS, double sigmaR)
{
    const int r = std::max(1, int(std::ceil(2.0 * sigmaS)));
    const int side = 2 * r + 1;
    std::vector<double> ws(std::size_t(side * side));
    for (int j = -r; j <= r; ++j)
        for (int i = -r; i <= r; ++i)
            ws[std::size_t((j + r) * side + i + r)] = std::exp(-0.5 * (i * i + j * j) / (sigmaS * sigmaS));
    const double inv = 1.0 / (2.0 * sigmaR * sigmaR);
    const ExpTable& ex = expTable();
    std::vector<double> dst(src.size());
    parallelRows(h, [&](int y0, int y1) {
        for (int y = y0; y < y1; ++y) {
            for (int x = 0; x < w; ++x) {
                const double c = src[std::size_t(y) * w + x];
                double acc = 0.0, wsum = 0.0;
                for (int j = -r; j <= r; ++j) {
                    const double* row = &src[std::size_t(clampi(y + j, 0, h - 1)) * w];
                    const double* wr = &ws[std::size_t((j + r) * side)];
                    for (int i = -r; i <= r; ++i) {
                        const double v = row[clampi(x + i, 0, w - 1)];
                        const double d = v - c;
                        const double wt = wr[i + r] * ex(d * d * inv);
                        acc += wt * v;
                        wsum += wt;
                    }
                }
                dst[std::size_t(y) * w + x] = wsum > 0.0 ? acc / wsum : c;
            }
        }
    });
    return dst;
}

// Non-local means (Buades et al.), pixel-wise, with the usual correction of
// the patch distance for the noise variance.
std::vector<double> nonLocalMeans(const std::vector<double>& src, int w, int h, int search, int patch, double sigma,
                                  double hParam)
{
    const int pad = search + patch;
    const int pw = w + 2 * pad, ph = h + 2 * pad;
    std::vector<double> p(std::size_t(pw) * ph);
    for (int y = 0; y < ph; ++y) {
        const double* row = &src[std::size_t(clampi(y - pad, 0, h - 1)) * w];
        double* out = &p[std::size_t(y) * pw];
        for (int x = 0; x < pw; ++x)
            out[x] = row[clampi(x - pad, 0, w - 1)];
    }
    const double patchN = double((2 * patch + 1) * (2 * patch + 1));
    const double h2 = std::max(1e-300, hParam * hParam);
    const double twoSigma2 = 2.0 * sigma * sigma;
    const ExpTable& ex = expTable();
    std::vector<double> dst(src.size());
    parallelRows(h, [&](int y0, int y1) {
        for (int y = y0; y < y1; ++y) {
            for (int x = 0; x < w; ++x) {
                const int cx = x + pad, cy = y + pad;
                double acc = 0.0, wsum = 0.0, wmax = 0.0;
                for (int dy = -search; dy <= search; ++dy) {
                    for (int dx = -search; dx <= search; ++dx) {
                        if (dx == 0 && dy == 0)
                            continue;
                        double d2 = 0.0;
                        for (int j = -patch; j <= patch; ++j) {
                            const double* a = &p[std::size_t(cy + j) * pw + cx - patch];
                            const double* b = &p[std::size_t(cy + dy + j) * pw + cx + dx - patch];
                            for (int i = 0; i <= 2 * patch; ++i) {
                                const double d = a[i] - b[i];
                                d2 += d * d;
                            }
                        }
                        const double wt = ex(std::max(0.0, d2 / patchN - twoSigma2) / h2);
                        acc += wt * p[std::size_t(cy + dy) * pw + cx + dx];
                        wsum += wt;
                        if (wt > wmax)
                            wmax = wt;
                    }
                }
                // The centre pixel gets the largest weight found among the others.
                if (wmax <= 0.0)
                    wmax = 1.0;
                acc += wmax * p[std::size_t(cy) * pw + cx];
                wsum += wmax;
                dst[std::size_t(y) * w + x] = acc / wsum;
            }
        }
    });
    return dst;
}

// Gaussian pyramid steps with the 1-4-6-4-1 kernel (for exposure fusion).
struct Level {
    std::vector<double> d;
    int w = 0;
    int h = 0;
};

Level pyrDown(const std::vector<double>& src, int w, int h)
{
    const int w2 = (w + 1) >> 1, h2 = (h + 1) >> 1;
    std::vector<double> tmp(std::size_t(w2) * h);
    Level out;
    out.w = w2;
    out.h = h2;
    out.d.resize(std::size_t(w2) * h2);
    for (int y = 0; y < h; ++y) {
        const double* row = &src[std::size_t(y) * w];
        for (int i = 0; i < w2; ++i) {
            const int c = 2 * i;
            tmp[std::size_t(y) * w2 + i] = (row[clampi(c - 2, 0, w - 1)] + 4 * row[clampi(c - 1, 0, w - 1)] + 6 * row[c]
                                            + 4 * row[clampi(c + 1, 0, w - 1)] + row[clampi(c + 2, 0, w - 1)])
                                           / 16.0;
        }
    }
    for (int j = 0; j < h2; ++j) {
        const int c = 2 * j;
        const double* r0 = &tmp[std::size_t(clampi(c - 2, 0, h - 1)) * w2];
        const double* r1 = &tmp[std::size_t(clampi(c - 1, 0, h - 1)) * w2];
        const double* r2 = &tmp[std::size_t(c) * w2];
        const double* r3 = &tmp[std::size_t(clampi(c + 1, 0, h - 1)) * w2];
        const double* r4 = &tmp[std::size_t(clampi(c + 2, 0, h - 1)) * w2];
        for (int i = 0; i < w2; ++i)
            out.d[std::size_t(j) * w2 + i] = (r0[i] + 4 * r1[i] + 6 * r2[i] + 4 * r3[i] + r4[i]) / 16.0;
    }
    return out;
}

std::vector<double> pyrUp(const std::vector<double>& src, int w, int h, int W2, int H2)
{
    std::vector<double> tmp(std::size_t(W2) * h), dst(std::size_t(W2) * H2);
    for (int y = 0; y < h; ++y) {
        const double* row = &src[std::size_t(y) * w];
        for (int X = 0; X < W2; ++X) {
            const int i = std::min(X >> 1, w - 1), ip = std::min(i + 1, w - 1), im = std::max(i - 1, 0);
            tmp[std::size_t(y) * W2 + X] = (X & 1) ? (row[i] + row[ip]) / 2.0 : (row[im] + 6 * row[i] + row[ip]) / 8.0;
        }
    }
    for (int Y = 0; Y < H2; ++Y) {
        const int j = std::min(Y >> 1, h - 1), jp = std::min(j + 1, h - 1), jm = std::max(j - 1, 0);
        const double* a = &tmp[std::size_t(jm) * W2];
        const double* b = &tmp[std::size_t(j) * W2];
        const double* c = &tmp[std::size_t(jp) * W2];
        double* o = &dst[std::size_t(Y) * W2];
        if (Y & 1)
            for (int X = 0; X < W2; ++X)
                o[X] = (b[X] + c[X]) / 2.0;
        else
            for (int X = 0; X < W2; ++X)
                o[X] = (a[X] + 6 * b[X] + c[X]) / 8.0;
    }
    return dst;
}

// ===========================================================================
// Stages
// ===========================================================================

// Parameter values of the selected algorithm, by id.
struct Params {
    const AlgorithmSpec* spec = nullptr;
    const QVector<double>* values = nullptr;

    double operator()(const char* id) const
    {
        const int i = paramIndex(*spec, id);
        return i < 0 ? 0.0 : (*values)[i];
    }
    int asInt(const char* id) const { return int(std::lround((*this)(id))); }
    bool is(const char* algorithmId) const { return std::strcmp(spec->id, algorithmId) == 0; }
};

QString translated(const char* source) { return QCoreApplication::translate("iat::isp", source); }
QString num(double v) { return QString::number(v, 'g', 5); }

// --- Black level -----------------------------------------------------------

QString blackLevel(Buffer& b, const Params& p)
{
    double level = 0.0;
    if (p.is("fixed")) {
        level = p("offset");
    } else {
        std::vector<double> v = b.d;
        level = quantileInPlace(v, p("pct") / 100.0);
    }
    const bool clip = p.asInt("clip") == 0;
    for (double& v : b.d) {
        v -= level;
        if (clip && v < 0.0)
            v = 0.0;
    }
    return translated(QT_TRANSLATE_NOOP("iat::isp", "Subtracted %1 DN")).arg(num(level));
}

// --- Defect pixels ---------------------------------------------------------

QString defectPixels(Buffer& b, const Params& p)
{
    const double thr = p("k") * noiseSigma(b);
    const int type = p.asInt("type");
    const bool hot = type != 2, dead = type != 1;
    const bool minmax = p.is("minmax");
    const int w = b.w, h = b.h;
    const std::vector<double> src = b.d;
    std::vector<qint64> counts(std::size_t(h), 0);
    parallelRows(h, [&](int y0, int y1) {
        std::array<double, 8> nb;
        for (int y = y0; y < y1; ++y) {
            for (int x = 0; x < w; ++x) {
                std::size_t k = 0;
                for (int j = -1; j <= 1; ++j) {
                    const int yy = y + j;
                    if (yy < 0 || yy >= h)
                        continue;
                    for (int i = -1; i <= 1; ++i) {
                        const int xx = x + i;
                        if ((i == 0 && j == 0) || xx < 0 || xx >= w)
                            continue;
                        nb[k++] = src[std::size_t(yy) * w + xx];
                    }
                }
                if (k < 3)
                    continue;
                const double c = src[std::size_t(y) * w + x];
                std::sort(nb.begin(), nb.begin() + std::ptrdiff_t(k));
                const double med = (k & 1) ? nb[k / 2] : 0.5 * (nb[k / 2 - 1] + nb[k / 2]);
                // In busy parts of the picture (texture, edges) the pixel must
                // stand out by more: the threshold grows by the spread of the
                // neighbours, without their own two extremes.
                const double t = thr + (k >= 5 ? nb[k - 2] - nb[1] : nb[k - 1] - nb[0]);
                bool bad = false;
                if (minmax)
                    bad = (hot && c > nb[k - 1] + t) || (dead && c < nb[0] - t);
                else
                    bad = (hot && c > med + t) || (dead && c < med - t);
                if (bad) {
                    b.d[std::size_t(y) * w + x] = med;
                    ++counts[std::size_t(y)];
                }
            }
        }
    });
    qint64 total = 0;
    for (qint64 c : counts)
        total += c;
    return translated(QT_TRANSLATE_NOOP("iat::isp", "%1 pixels corrected (%2 %)"))
        .arg(total)
        .arg(QString::number(100.0 * double(total) / double(std::max<std::size_t>(1, b.n())), 'g', 2));
}

// --- Defect rows / columns -------------------------------------------------

Buffer transposed(const Buffer& b)
{
    Buffer t;
    t.w = b.h;
    t.h = b.w;
    t.d.resize(b.n());
    for (int y = 0; y < b.h; ++y)
        for (int x = 0; x < b.w; ++x)
            t.d[std::size_t(x) * t.w + y] = b.d[std::size_t(y) * b.w + x];
    return t;
}

// Median of the values that lie within 3 σ (robust) of the plain median.
// One part of a row can sit on a different structure than the rest (an edge
// in the picture); this keeps the larger, consistent part. Reorders `v`.
double clippedMedian(std::vector<double>& v)
{
    if (v.empty())
        return 0.0;
    const double med = medianInPlace(v);
    std::vector<double> dev(v.size());
    for (std::size_t i = 0; i < v.size(); ++i)
        dev[i] = std::abs(v[i] - med);
    const double limit = 3.0 * 1.4826 * medianInPlace(dev);
    std::vector<double> kept;
    kept.reserve(v.size());
    for (double x : v)
        if (std::abs(x - med) <= limit)
            kept.push_back(x);
    return kept.empty() ? med : medianInPlace(kept);
}

// How far every row is offset from the rows around it (for destriping).
//
// Pixel by pixel, the reference is the median of the 2·half + 1 pixels above
// and below (the row itself included). The offset of a row is the median
// over its pixels of (pixel - reference). Working pixel by pixel takes the
// picture itself out of the estimate, so it stays accurate on images with
// strong structure along the row. The first and last `half` rows continue
// the reference next to them in a straight line.
std::vector<double> rowOffsets(const Buffer& b, int half)
{
    const int w = b.w, n = b.h;
    std::vector<double> off(static_cast<std::size_t>(n), 0.0);
    half = std::max(1, std::min(half, (n - 1) / 4));
    if (n < 5 || w < 1)
        return off;

    const int first = half, last = n - half - 1;
    std::vector<double> S(b.n());
    parallelRows(n, [&](int y0, int y1) {
        std::vector<double> v(std::size_t(2 * half + 1));
        for (int y = std::max(y0, first); y < std::min(y1, last + 1); ++y) {
            for (int x = 0; x < w; ++x) {
                for (int j = -half; j <= half; ++j)
                    v[std::size_t(j + half)] = b.d[std::size_t(y + j) * w + x];
                std::nth_element(v.begin(), v.begin() + half, v.end());
                S[std::size_t(y) * w + x] = v[std::size_t(half)];
            }
        }
    });
    const int span = std::min(half, last - first);
    for (int x = 0; x < w; ++x) {
        const double a0 = S[std::size_t(first) * w + x], a1 = S[std::size_t(first + span) * w + x];
        for (int y = 0; y < first; ++y)
            S[std::size_t(y) * w + x] = a0 + (a1 - a0) * double(y - first) / double(span);
        const double b0 = S[std::size_t(last) * w + x], b1 = S[std::size_t(last - span) * w + x];
        for (int y = last + 1; y < n; ++y)
            S[std::size_t(y) * w + x] = b0 + (b0 - b1) * double(y - last) / double(span);
    }
    parallelRows(n, [&](int y0, int y1) {
        std::vector<double> v(static_cast<std::size_t>(w));
        for (int y = y0; y < y1; ++y) {
            for (int x = 0; x < w; ++x)
                v[std::size_t(x)] = b.d[std::size_t(y) * w + x] - S[std::size_t(y) * w + x];
            off[std::size_t(y)] = clippedMedian(v);
        }
    });
    return off;
}

// How far every row stands out from the rows on BOTH sides of it (for
// finding defective rows).
//
// Pixel by pixel, the row is compared with the median of the 5 pixels above
// and with the median of the 5 pixels below. Only what differs from both in
// the same direction counts, and then the smaller of the two differences:
// a defective row differs from both sides, while an edge in the picture
// agrees with one side and a brightness slope lies between the two. The
// offset of the row is the median of that over its pixels. The very first
// and last row have one side only: they are compared with the straight-line
// continuation of the six rows next to them. Needs at least 7 rows.
std::vector<double> rowDefectOffsets(const Buffer& b)
{
    const int w = b.w, n = b.h, side = 5;
    std::vector<double> off(static_cast<std::size_t>(n), 0.0);
    if (n < 7)
        return off;
    const auto med3 = [](double a, double c, double e) {
        return std::max(std::min(a, c), std::min(std::max(a, c), e));
    };
    // The end row `y`, continued from the rows y+s, y+2s, … y+6s (s = ±1).
    const auto endResidual = [&](int y, int s, int x) {
        const auto px = [&](int k) { return b.d[std::size_t(y + s * k) * w + x]; };
        const double close = med3(px(1), px(2), px(3)), further = med3(px(4), px(5), px(6));
        return px(0) - (close - 2.0 * (further - close) / 3.0);
    };
    parallelRows(n, [&](int y0, int y1) {
        std::vector<double> v(static_cast<std::size_t>(w));
        std::array<double, 5> up, dn;
        for (int y = y0; y < y1; ++y) {
            const int nu = std::min(side, y), nd = std::min(side, n - 1 - y);
            for (int x = 0; x < w; ++x) {
                const double c = b.d[std::size_t(y) * w + x];
                double du = 0.0, dd = 0.0;
                if (nu > 0) {
                    for (int j = 0; j < nu; ++j)
                        up[std::size_t(j)] = b.d[std::size_t(y - 1 - j) * w + x];
                    std::nth_element(up.begin(), up.begin() + nu / 2, up.begin() + nu);
                    du = c - up[std::size_t(nu / 2)];
                }
                if (nd > 0) {
                    for (int j = 0; j < nd; ++j)
                        dn[std::size_t(j)] = b.d[std::size_t(y + 1 + j) * w + x];
                    std::nth_element(dn.begin(), dn.begin() + nd / 2, dn.begin() + nd);
                    dd = c - dn[std::size_t(nd / 2)];
                }
                double r = 0.0;
                if (nu == 0)
                    r = endResidual(y, +1, x);
                else if (nd == 0)
                    r = endResidual(y, -1, x);
                else if ((du > 0.0) == (dd > 0.0))
                    r = std::abs(du) < std::abs(dd) ? du : dd;
                v[std::size_t(x)] = r;
            }
            off[std::size_t(y)] = clippedMedian(v);
        }
    });
    return off;
}

// Finds rows that stand out from their neighbours and rebuilds them from
// the nearest good rows. Returns how many rows were replaced.
//
// A defective row also disturbs what is predicted for the rows next to it,
// so the search goes in rounds: the worst rows are repaired first, then the
// rest is judged again on the repaired image.
int replaceRows(Buffer& b, double k, double sigma)
{
    const int lines = b.h, len = b.w;
    if (lines < 8)
        return 0;
    const std::vector<double> original = b.d;
    std::vector<char> bad(static_cast<std::size_t>(lines), 0);
    int count = 0;
    for (int round = 0; round < 8; ++round) {
        const std::vector<double> d = rowDefectOffsets(b);
        std::vector<double> dev(d.size());
        std::vector<double> tmp = d;
        const double centre = medianInPlace(tmp);
        for (std::size_t i = 0; i < d.size(); ++i)
            dev[i] = std::abs(d[i] - centre);
        tmp = dev;
        // A defective row stands out by k times the spread of the row
        // offsets (robust; never below what the pixel noise alone gives the
        // median of a row), and by more than the pixel noise itself, so
        // that it is a visible line and not a statistical fluke.
        const double spread = std::max(1.4826 * medianInPlace(tmp), 1.5 * sigma / std::sqrt(double(len)));
        const double threshold = std::max(k * spread, sigma);
        double worst = 0.0;
        for (std::size_t i = 0; i < d.size(); ++i)
            if (!bad[i])
                worst = std::max(worst, dev[i]);
        if (!(worst > threshold))
            break;
        const double limit = std::max(threshold, 0.5 * worst);
        for (std::size_t i = 0; i < d.size(); ++i) {
            if (!bad[i] && dev[i] > limit) {
                bad[i] = 1;
                ++count;
            }
        }
        if (count > lines / 4) {
            b.d = original;  // too many to trust the detection
            return 0;
        }
        for (int l = 0; l < lines; ++l) {
            if (!bad[std::size_t(l)])
                continue;
            int a = l - 1, c = l + 1;
            while (a >= 0 && bad[std::size_t(a)])
                --a;
            while (c < lines && bad[std::size_t(c)])
                ++c;
            double* row = &b.d[std::size_t(l) * len];
            const double* ra = a >= 0 ? &b.d[std::size_t(a) * len] : nullptr;
            const double* rc = c < lines ? &b.d[std::size_t(c) * len] : nullptr;
            const double t = ra && rc ? double(l - a) / double(c - a) : 0.0;
            for (int i = 0; i < len; ++i)
                row[i] = !ra ? rc[i] : !rc ? ra[i] : ra[i] + (rc[i] - ra[i]) * t;
        }
    }
    return count;
}

void destripeRows(Buffer& b, int window, double strength)
{
    const std::vector<double> off = rowOffsets(b, window / 2);
    for (int y = 0; y < b.h; ++y) {
        const double o = strength * off[std::size_t(y)];
        if (o == 0.0)
            continue;
        double* row = &b.d[std::size_t(y) * b.w];
        for (int x = 0; x < b.w; ++x)
            row[x] -= o;
    }
}

QString defectLines(Buffer& b, const Params& p)
{
    const int dir = p.asInt("dir");
    const bool doRows = dir != 2, doCols = dir != 1;
    const bool replace = p.is("replace");
    const double sigma = replace ? noiseSigma(b) : 0.0;
    int rows = 0, cols = 0;
    if (doRows) {
        if (replace)
            rows = replaceRows(b, p("k"), sigma);
        else
            destripeRows(b, p.asInt("window"), p("strength"));
    }
    if (doCols) {
        // Columns are the rows of the transposed image.
        Buffer t = transposed(b);
        if (replace)
            cols = replaceRows(t, p("k"), sigma);
        else
            destripeRows(t, p.asInt("window"), p("strength"));
        b = transposed(t);
    }
    if (replace)
        return translated(QT_TRANSLATE_NOOP("iat::isp", "%1 rows and %2 columns replaced")).arg(rows).arg(cols);
    return {};
}

// --- Digital gain ----------------------------------------------------------

QString digitalGain(Buffer& b, const Params& p)
{
    double gain = 1.0;
    if (p.is("manual")) {
        gain = p("gain");
    } else {
        double sum = 0.0;
        for (double v : b.d)
            sum += v;
        const double mean = sum / double(std::max<std::size_t>(1, b.n()));
        if (mean > 0.0)
            gain = p("target") / mean;
    }
    for (double& v : b.d)
        v *= gain;
    return translated(QT_TRANSLATE_NOOP("iat::isp", "Gain × %1")).arg(num(gain));
}

// --- Denoise ---------------------------------------------------------------

QString denoise(Buffer& b, const Params& p)
{
    const double sigma = noiseSigma(b);
    if (p.is("gaussian"))
        b.d = gaussianBlur(b.d, b.w, b.h, p("sigma"));
    else if (p.is("median"))
        b.d = medianFilter(b.d, b.w, b.h, p.asInt("size") == 0 ? 1 : 2);
    else if (p.is("bilateral"))
        b.d = bilateral(b.d, b.w, b.h, p("sigma_s"), p("k") * sigma);
    else if (p.is("guided")) {
        const double e = p("k") * sigma;
        b.d = guidedSelf(b.d, b.w, b.h, p.asInt("radius"), e * e);
    } else if (p.is("nlm"))
        b.d = nonLocalMeans(b.d, b.w, b.h, p.asInt("search"), p.asInt("patch") == 0 ? 1 : 2, sigma, p("k") * sigma);
    const double after = noiseSigma(b);
    return translated(QT_TRANSLATE_NOOP("iat::isp", "Noise σ about %1, then %2")).arg(num(sigma), num(after));
}

// --- Dynamic range compression (result in the input's units) ----------------

// The local operator shared by dynamic range compression and tone mapping:
// returns log2 of the compressed image, relative to the robust maximum.
std::vector<double> compressBase(const std::vector<double>& v, int w, int h, double rangeStops, double detailGain,
                                 int radius, double eps, double offsetStops)
{
    const std::size_t n = v.size();
    const double off = std::pow(2.0, -10.0);
    std::vector<double> L(n);
    for (std::size_t i = 0; i < n; ++i)
        L[i] = std::log2(v[i] + off);
    const std::vector<double> B = guidedSelf(L, w, h, radius, eps);
    const auto [mn, mx] = std::minmax_element(B.begin(), B.end());
    const double bmax = *mx;
    const double s = std::min(1.0, rangeStops / std::max(1e-6, bmax - *mn));
    std::vector<double> lg(n);
    for (std::size_t i = 0; i < n; ++i)
        lg[i] = (B[i] - bmax) * s - offsetStops + (L[i] - B[i]) * detailGain;
    return lg;
}

QString dynamicRange(Buffer& b, const Params& p)
{
    const Range r = robustRange(b);
    std::vector<double> v = normalised(b, r);
    if (p.is("local")) {
        const std::vector<double> lg = compressBase(v, b.w, b.h, p("range"), 1.0, p.asInt("radius"), p("eps"), 0.0);
        for (std::size_t i = 0; i < v.size(); ++i)
            v[i] = clamp01(std::pow(2.0, lg[i]));
    } else if (p.is("log")) {
        const double a = p("a"), k = 1.0 / std::log(1.0 + a);
        for (double& x : v)
            x = std::log(1.0 + a * x) * k;
    } else {
        const double e = 1.0 / p("g");
        for (double& x : v)
            x = std::pow(x, e);
    }
    for (std::size_t i = 0; i < v.size(); ++i)
        b.d[i] = r.lo + v[i] * (r.hi - r.lo);
    return translated(QT_TRANSLATE_NOOP("iat::isp", "Range %1 to %2 DN kept")).arg(num(r.lo), num(r.hi));
}

// --- Tone mapping (result 0..255) ------------------------------------------

const double kDisplayGamma = 1.0 / 2.2;

void curve(Buffer& b, const Range& r, const std::function<double(double)>& fn)
{
    const double s = 1.0 / (r.hi - r.lo);
    parallelRows(b.h, [&](int y0, int y1) {
        for (std::size_t i = std::size_t(y0) * b.w, e = std::size_t(y1) * b.w; i < e; ++i)
            b.d[i] = 255.0 * clamp01(fn(clamp01((b.d[i] - r.lo) * s)));
    });
}

void plateauEqualisation(Buffer& b, double plateau)
{
    const auto [mnIt, mxIt] = std::minmax_element(b.d.begin(), b.d.end());
    const double mn = *mnIt, mx = *mxIt;
    if (!(mx > mn)) {
        std::fill(b.d.begin(), b.d.end(), 0.0);
        return;
    }
    // One bin per whole number for integer data of modest range (as for raw
    // sensor data), otherwise 65536 equal bins.
    bool integerValued = (mx - mn) < 65536.0;
    for (std::size_t i = 0; integerValued && i < b.n(); ++i)
        integerValued = b.d[i] == std::floor(b.d[i]);
    const std::size_t bins = integerValued ? std::size_t(mx - mn) + 1 : 65536;
    const double scale = integerValued ? 1.0 : double(bins - 1) / (mx - mn);
    auto binOf = [&](double v) { return std::min(bins - 1, std::size_t((v - mn) * scale + 0.5)); };

    std::vector<double> hist(bins, 0.0);
    for (double v : b.d)
        hist[binOf(v)] += 1.0;
    std::size_t occupied = 0;
    for (double c : hist)
        if (c > 0.0)
            ++occupied;
    const double T = plateau >= 199.0 ? std::numeric_limits<double>::infinity()
                                      : std::max(1.0, plateau * double(b.n()) / double(occupied));
    double c = 0.0, first = -1.0;
    for (std::size_t i = 0; i < bins; ++i) {
        const double hgt = std::min(hist[i], T);
        if (hgt > 0.0 && first < 0.0)
            first = hgt;
        c += hgt;
        hist[i] = c;
    }
    const double den = std::max(1e-9, c - first);
    for (double& v : b.d)
        v = 255.0 * clamp01((hist[binOf(v)] - first) / den);
}

void clahe(Buffer& b, int tiles, double clip)
{
    const int w = b.w, h = b.h;
    const std::size_t n = b.n();
    const std::vector<double> v = normalised(b, robustRange(b));
    const int bins = 1024;
    const double k = 1.0 / std::log(201.0);
    std::vector<quint16> bi(n);
    for (std::size_t i = 0; i < n; ++i)
        bi[i] = quint16(std::min(bins - 1, int(std::log(1.0 + 200.0 * v[i]) * k * bins)));
    const int t = std::max(1, std::min({tiles, w, h}));
    const double tw = double(w) / t, th = double(h) / t;
    std::vector<double> luts(std::size_t(t) * t * bins);
    std::vector<double> hist(static_cast<std::size_t>(bins));
    for (int ty = 0; ty < t; ++ty) {
        for (int tx = 0; tx < t; ++tx) {
            std::fill(hist.begin(), hist.end(), 0.0);
            const int x0 = int(std::floor(tx * tw)), x1 = int(std::floor((tx + 1) * tw));
            const int y0 = int(std::floor(ty * th)), y1 = int(std::floor((ty + 1) * th));
            for (int y = y0; y < y1; ++y)
                for (int x = x0; x < x1; ++x)
                    hist[bi[std::size_t(y) * w + x]] += 1.0;
            const double np = std::max(1.0, double(x1 - x0) * double(y1 - y0));
            const double limit = std::max(1.0, clip * np / bins);
            double excess = 0.0;
            for (double& c : hist) {
                if (c > limit) {
                    excess += c - limit;
                    c = limit;
                }
            }
            const double add = excess / bins;
            double* lut = &luts[(std::size_t(ty) * t + tx) * bins];
            double c = 0.0;
            for (int i = 0; i < bins; ++i) {
                c += hist[std::size_t(i)] + add;
                lut[i] = c / np;
            }
        }
    }
    parallelRows(h, [&](int ya, int yb) {
        for (int y = ya; y < yb; ++y) {
            const double gy = std::clamp((y + 0.5) / th - 0.5, 0.0, double(t - 1));
            const int j0 = int(std::floor(gy)), j1 = std::min(j0 + 1, t - 1);
            const double fy = gy - j0;
            for (int x = 0; x < w; ++x) {
                const double gx = std::clamp((x + 0.5) / tw - 0.5, 0.0, double(t - 1));
                const int i0 = int(std::floor(gx)), i1 = std::min(i0 + 1, t - 1);
                const double fx = gx - i0;
                const int bin = bi[std::size_t(y) * w + x];
                const double a = luts[(std::size_t(j0) * t + i0) * bins + bin];
                const double bb = luts[(std::size_t(j0) * t + i1) * bins + bin];
                const double c = luts[(std::size_t(j1) * t + i0) * bins + bin];
                const double d = luts[(std::size_t(j1) * t + i1) * bins + bin];
                b.d[std::size_t(y) * w + x] = 255.0 * clamp01((a + (bb - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy);
            }
        }
    });
}

void exposureFusion(Buffer& b, int K, double span, double sigma)
{
    const int w = b.w, h = b.h;
    const std::size_t n = b.n();
    const std::vector<double> v = normalised(b, robustRange(b));
    K = std::max(2, K);
    const double i2s = 1.0 / (2.0 * sigma * sigma);

    // The virtual exposures and their well-exposedness weights.
    std::vector<std::vector<double>> exposures(static_cast<std::size_t>(K));
    std::vector<double> sumW(n, 0.0);
    for (int e = 0; e < K; ++e) {
        const double gain = std::pow(2.0, span * e / (K - 1));
        std::vector<double>& E = exposures[std::size_t(e)];
        E.resize(n);
        parallelRows(h, [&](int y0, int y1) {
            for (std::size_t i = std::size_t(y0) * w, end = std::size_t(y1) * w; i < end; ++i)
                E[i] = std::pow(clamp01(v[i] * gain), kDisplayGamma);
        });
        for (std::size_t i = 0; i < n; ++i) {
            const double d = E[i] - 0.5;
            sumW[i] += std::exp(-d * d * i2s) + 1e-6;
        }
    }

    std::vector<std::pair<int, int>> sizes{{w, h}};
    while (std::min(sizes.back().first, sizes.back().second) > 12)
        sizes.emplace_back((sizes.back().first + 1) >> 1, (sizes.back().second + 1) >> 1);
    std::vector<std::vector<double>> acc;
    for (const auto& s : sizes)
        acc.emplace_back(std::size_t(s.first) * s.second, 0.0);

    for (std::vector<double>& E : exposures) {
        std::vector<double> gw(n);
        for (std::size_t i = 0; i < n; ++i) {
            const double d = E[i] - 0.5;
            gw[i] = (std::exp(-d * d * i2s) + 1e-6) / sumW[i];
        }
        std::vector<double> ge = std::move(E);
        for (std::size_t l = 0; l < sizes.size(); ++l) {
            const int a = sizes[l].first, c = sizes[l].second;
            std::vector<double>& A = acc[l];
            if (l == sizes.size() - 1) {
                for (std::size_t i = 0; i < A.size(); ++i)
                    A[i] += gw[i] * ge[i];
                break;
            }
            Level de = pyrDown(ge, a, c), dw = pyrDown(gw, a, c);
            const std::vector<double> up = pyrUp(de.d, de.w, de.h, a, c);
            for (std::size_t i = 0; i < A.size(); ++i)
                A[i] += gw[i] * (ge[i] - up[i]);
            ge = std::move(de.d);
            gw = std::move(dw.d);
        }
    }
    std::vector<double> R = acc.back();
    for (std::size_t l = sizes.size() - 1; l-- > 0;) {
        std::vector<double> U = pyrUp(R, sizes[l + 1].first, sizes[l + 1].second, sizes[l].first, sizes[l].second);
        const std::vector<double>& A = acc[l];
        for (std::size_t i = 0; i < A.size(); ++i)
            U[i] += A[i];
        R = std::move(U);
    }
    for (std::size_t i = 0; i < n; ++i)
        b.d[i] = 255.0 * clamp01(R[i]);
}

QString toneMap(Buffer& b, const Params& p)
{
    if (p.is("shift")) {
        static const int depths[] = {0, 8, 10, 12, 14, 16};
        int bits = depths[clampi(p.asInt("bits"), 0, 5)];
        if (bits == 0) {
            const double mx = *std::max_element(b.d.begin(), b.d.end());
            bits = 8;
            while (bits < 16 && mx >= std::pow(2.0, bits))
                bits += 2;
        }
        const double div = std::pow(2.0, bits - 8);
        for (double& v : b.d)
            v = std::clamp(std::floor(v / div), 0.0, 255.0);
        return translated(QT_TRANSLATE_NOOP("iat::isp", "%1-bit input: divided by %2")).arg(bits).arg(num(div));
    }
    if (p.is("he")) {
        plateauEqualisation(b, p("plateau"));
        return {};
    }
    if (p.is("clahe")) {
        clahe(b, p.asInt("tiles"), p("clip"));
        return {};
    }
    if (p.is("fusion")) {
        exposureFusion(b, p.asInt("k"), p("span"), p("sigma"));
        return {};
    }

    Range r = robustRange(b);
    if (p.is("stretch")) {
        r = quantileRange(b, p("lo") / 100.0, 1.0 - p("hi") / 100.0);
        if (!(r.hi > r.lo))
            r.hi = r.lo + 1.0;
        curve(b, r, [](double v) { return v; });
    } else if (p.is("gamma")) {
        const double e = 1.0 / p("g");
        curve(b, r, [e](double v) { return std::pow(v, e); });
    } else if (p.is("log")) {
        const double a = p("a"), k = 1.0 / std::log(1.0 + a);
        curve(b, r, [a, k](double v) { return std::log(1.0 + a * v) * k; });
    } else if (p.is("reinhard")) {
        const double s = 1.0 / (r.hi - r.lo);
        double acc = 0.0;
        for (double v : b.d)
            acc += std::log(clamp01((v - r.lo) * s) + 1e-4);
        const double k = p("key") / std::exp(acc / double(b.n()));
        const double lw = k * p("white"), lw2 = lw * lw;
        curve(b, r, [k, lw2](double v) {
            const double L = v * k;
            return std::pow(clamp01(L * (1.0 + L / lw2) / (1.0 + L)), kDisplayGamma);
        });
    } else if (p.is("local")) {
        const std::vector<double> v = normalised(b, r);
        const std::vector<double> lg =
            compressBase(v, b.w, b.h, p("range"), p("detail"), p.asInt("radius"), p("eps"), 0.2);
        for (std::size_t i = 0; i < v.size(); ++i)
            b.d[i] = 255.0 * std::pow(clamp01(std::pow(2.0, lg[i])), kDisplayGamma);
    }
    return translated(QT_TRANSLATE_NOOP("iat::isp", "Input window %1 to %2 DN")).arg(num(r.lo), num(r.hi));
}

// --- Detail enhancement ----------------------------------------------------

QString detail(Buffer& b, const Params& p)
{
    const std::size_t n = b.n();
    if (p.is("guided")) {
        const double sigma = noiseSigma(b);
        const double e = p("k") * sigma;
        const std::vector<double> base = guidedSelf(b.d, b.w, b.h, p.asInt("radius"), e * e);
        const double gain = p("gain"), t = p("core") * sigma;
        for (std::size_t i = 0; i < n; ++i) {
            const double d = b.d[i] - base[i];
            // The extra gain fades in between 1× and 2× the noise threshold.
            double f = 1.0;
            if (t > 0.0) {
                const double x = clamp01((std::abs(d) - t) / t);
                f = x * x * (3.0 - 2.0 * x);
            }
            b.d[i] = base[i] + d * (1.0 + (gain - 1.0) * f);
        }
        return {};
    }
    const std::vector<double> g1 = gaussianBlur(b.d, b.w, b.h, 1.0);
    const std::vector<double> g4 = gaussianBlur(g1, b.w, b.h, std::sqrt(16.0 - 1.0));
    const std::vector<double> g16 = gaussianBlur(g4, b.w, b.h, std::sqrt(256.0 - 16.0));
    const double gf = p("fine"), gm = p("medium"), gc = p("coarse");
    for (std::size_t i = 0; i < n; ++i)
        b.d[i] = g16[i] + gc * (g4[i] - g16[i]) + gm * (g1[i] - g4[i]) + gf * (b.d[i] - g1[i]);
    return {};
}

// --- Sharpening ------------------------------------------------------------

QString sharpen(Buffer& b, const Params& p)
{
    const int w = b.w, h = b.h;
    const std::vector<double> src = b.d;
    if (p.is("laplace")) {
        const double a = p("amount");
        parallelRows(h, [&](int y0, int y1) {
            for (int y = y0; y < y1; ++y) {
                const double* up = &src[std::size_t(std::max(0, y - 1)) * w];
                const double* row = &src[std::size_t(y) * w];
                const double* dn = &src[std::size_t(std::min(h - 1, y + 1)) * w];
                for (int x = 0; x < w; ++x) {
                    const double lap = up[x] + dn[x] + row[std::max(0, x - 1)] + row[std::min(w - 1, x + 1)] - 4 * row[x];
                    b.d[std::size_t(y) * w + x] = row[x] - a * lap;
                }
            }
        });
        return {};
    }
    const double t = p("thr") * noiseSigma(b);
    const double amount = p("amount");
    const bool limit = p.asInt("limit") == 1;
    const std::vector<double> blur = gaussianBlur(src, w, h, p("sigma"));
    parallelRows(h, [&](int y0, int y1) {
        for (int y = y0; y < y1; ++y) {
            for (int x = 0; x < w; ++x) {
                const std::size_t i = std::size_t(y) * w + x;
                double d = src[i] - blur[i];
                // Soft threshold: differences at noise level are not amplified.
                if (t > 0.0)
                    d = d > t ? d - t : d < -t ? d + t : 0.0;
                double v = src[i] + amount * d;
                if (limit) {
                    double mn = src[i], mx = src[i];
                    for (int j = -1; j <= 1; ++j) {
                        const double* row = &src[std::size_t(clampi(y + j, 0, h - 1)) * w];
                        for (int k = -1; k <= 1; ++k) {
                            const double s = row[clampi(x + k, 0, w - 1)];
                            mn = std::min(mn, s);
                            mx = std::max(mx, s);
                        }
                    }
                    v = std::clamp(v, mn, mx);
                }
                b.d[i] = v;
            }
        }
    });
    return {};
}

// --- Output quantisation (display values 0..255) -----------------------------

// Small deterministic generator, so a dithered result is reproducible.
struct Mulberry32 {
    quint32 a;
    double operator()()
    {
        a += 0x6D2B79F5u;
        quint32 t = (a ^ (a >> 15)) * (1u | a);
        t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
        return double(t ^ (t >> 14)) / 4294967296.0;
    }
};

std::array<double, 64> bayerMatrix()
{
    std::vector<std::vector<int>> m{{0}};
    for (int s = 1; s < 8; s *= 2) {
        const std::size_t n = m.size();
        std::vector<std::vector<int>> r(2 * n, std::vector<int>(2 * n));
        for (std::size_t y = 0; y < n; ++y) {
            for (std::size_t x = 0; x < n; ++x) {
                const int v = 4 * m[y][x];
                r[y][x] = v;
                r[y][x + n] = v + 2;
                r[y + n][x] = v + 3;
                r[y + n][x + n] = v + 1;
            }
        }
        m = r;
    }
    std::array<double, 64> out{};
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            out[std::size_t(y * 8 + x)] = (m[std::size_t(y)][std::size_t(x)] + 0.5) / 64.0 - 0.5;
    return out;
}

QString quantise(Buffer& b, const Params& p)
{
    static const int depths[] = {8, 6, 4};
    const int bits = depths[clampi(p.asInt("bits"), 0, 2)];
    const double L = double((1 << bits) - 1), sc = 255.0 / L;
    const int w = b.w, h = b.h;
    if (p.is("fs")) {
        std::vector<double> err(std::size_t(w) + 2, 0.0), nxt(std::size_t(w) + 2, 0.0);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const std::size_t i = std::size_t(y) * w + x;
                const double v = b.d[i] / 255.0 * L + err[std::size_t(x) + 1];
                const double q = std::clamp(std::round(v), 0.0, L);
                const double e = v - q;
                err[std::size_t(x) + 2] += e * 0.4375;
                nxt[std::size_t(x)] += e * 0.1875;
                nxt[std::size_t(x) + 1] += e * 0.3125;
                nxt[std::size_t(x) + 2] += e * 0.0625;
                b.d[i] = std::round(q * sc);
            }
            err.swap(nxt);
            std::fill(nxt.begin(), nxt.end(), 0.0);
        }
    } else {
        static const std::array<double, 64> bayer = bayerMatrix();
        const bool ordered = p.is("bayer"), random = p.is("noise");
        Mulberry32 rng{7};
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const std::size_t i = std::size_t(y) * w + x;
                double d = 0.0;
                if (ordered)
                    d = bayer[std::size_t((y & 7) * 8 + (x & 7))];
                else if (random)
                    d = rng() + rng() - 1.0;
                const double q = std::clamp(std::floor(b.d[i] / 255.0 * L + 0.5 + d), 0.0, L);
                b.d[i] = std::round(q * sc);
            }
        }
    }
    return translated(QT_TRANSLATE_NOOP("iat::isp", "%1 grey levels")).arg(1 << bits);
}

Buffer toBuffer(const Image2D& image)
{
    Buffer b;
    b.w = image.width();
    b.h = image.height();
    b.d.assign(image.constData(), image.constData() + image.pixelCount());
    return b;
}

} // namespace

// ===========================================================================
// Running the pipeline
// ===========================================================================

double estimateNoise(const Image2D& image)
{
    if (image.isNull())
        return 0.0;
    Buffer b = toBuffer(image);
    for (double& v : b.d)
        if (!std::isfinite(v))
            v = 0.0;
    return noiseSigma(b);
}

Result run(const Image2D& input, const Pipeline& pipeline)
{
    Result result;
    result.image = input;
    for (std::size_t i = 0; i < stages().size(); ++i)
        result.notes << QString();
    if (input.isNull() || !pipeline.isActive())
        return result;

    QElapsedTimer timer;
    timer.start();

    Buffer b = toBuffer(input);

    // NaN / Inf pixels would spread through every filter. They take the
    // median of the finite pixels while processing and are put back after.
    std::vector<std::size_t> nonFinite;
    for (std::size_t i = 0; i < b.n(); ++i)
        if (!std::isfinite(b.d[i]))
            nonFinite.push_back(i);
    if (nonFinite.size() == b.n())
        return result;
    if (!nonFinite.empty()) {
        std::vector<double> finite;
        finite.reserve(b.n() - nonFinite.size());
        for (double v : b.d)
            if (std::isfinite(v))
                finite.push_back(v);
        const double fill = medianInPlace(finite);
        for (std::size_t i : nonFinite)
            b.d[i] = fill;
    }

    const bool display = pipeline.isEnabled(StageId::ToneMap);
    for (const StageSpec& spec : stages()) {
        if (!pipeline.isEnabled(spec.id))
            continue;
        const StageConfig& cfg = pipeline.config(spec.id);
        Params p;
        p.spec = pipeline.algorithm(spec.id);
        if (!p.spec)
            continue;
        p.values = &cfg.values[cfg.algorithm];

        QString note;
        switch (spec.id) {
        case StageId::BlackLevel:   note = blackLevel(b, p); break;
        case StageId::DefectPixel:  note = defectPixels(b, p); break;
        case StageId::DefectLine:   note = defectLines(b, p); break;
        case StageId::Gain:         note = digitalGain(b, p); break;
        case StageId::Denoise:      note = denoise(b, p); break;
        case StageId::DynamicRange: note = dynamicRange(b, p); break;
        case StageId::ToneMap:      note = toneMap(b, p); break;
        case StageId::Detail:       note = detail(b, p); break;
        case StageId::Sharpen:      note = sharpen(b, p); break;
        case StageId::Output:
            if (display) {
                for (double& v : b.d)
                    v = std::clamp(v, 0.0, 255.0);
                note = quantise(b, p);
            } else {
                note = translated(QT_TRANSLATE_NOOP("iat::isp", "Skipped: turn on tone mapping first"));
            }
            break;
        }
        result.notes[stageIndex(spec.id)] = note;
    }

    // Detail enhancement and sharpening can push display values past the ends.
    if (display)
        for (double& v : b.d)
            v = std::clamp(v, 0.0, 255.0);

    Image2D out(b.w, b.h);
    std::copy(b.d.begin(), b.d.end(), out.data());
    if (!nonFinite.empty()) {
        double* o = out.data();
        const double* in = input.constData();
        for (std::size_t i : nonFinite)
            o[i] = in[i];
    }
    result.image = out;
    result.displayReferred = display;
    result.milliseconds = double(timer.nsecsElapsed()) / 1e6;
    return result;
}

// ===========================================================================
// Presets
// ===========================================================================

const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        {"general", QT_TRANSLATE_NOOP("iat::isp", "Ordinary scene"),
         QT_TRANSLATE_NOOP("iat::isp", "Defect pixel correction and a gamma curve: monotonic and predictable.")},
        {"hdr", QT_TRANSLATE_NOOP("iat::isp", "Bright and dark regions in one frame"),
         QT_TRANSLATE_NOOP("iat::isp", "Light denoising and exposure fusion, which compresses large-scale contrast "
                                       "and leaves texture alone.")},
        {"narrow", QT_TRANSLATE_NOOP("iat::isp", "Low contrast on a pedestal (thermal)"),
         QT_TRANSLATE_NOOP("iat::isp", "Destriping, plateau equalisation and a detail boost.")},
        {"inspection", QT_TRANSLATE_NOOP("iat::isp", "Inspection: visibility first"),
         QT_TRANSLATE_NOOP("iat::isp", "Bilateral denoising and CLAHE, for the most local contrast everywhere.")},
        {"cleanup", QT_TRANSLATE_NOOP("iat::isp", "Sensor clean-up only"),
         QT_TRANSLATE_NOOP("iat::isp", "Defect pixels and defect lines are repaired; the values stay in DN.")},
    };
    return p;
}

Pipeline presetPipeline(const char* presetId)
{
    Pipeline p;
    const auto is = [presetId](const char* id) { return std::strcmp(presetId, id) == 0; };
    if (is("general")) {
        p.setEnabled(StageId::DefectPixel, true);
        p.setEnabled(StageId::ToneMap, true);
        p.setAlgorithm(StageId::ToneMap, "gamma");
    } else if (is("hdr")) {
        p.setEnabled(StageId::DefectPixel, true);
        p.setEnabled(StageId::Denoise, true);
        p.setAlgorithm(StageId::Denoise, "guided");
        p.setValue(StageId::Denoise, "radius", 2);
        p.setEnabled(StageId::ToneMap, true);
        p.setAlgorithm(StageId::ToneMap, "fusion");
    } else if (is("narrow")) {
        p.setEnabled(StageId::DefectPixel, true);
        p.setEnabled(StageId::DefectLine, true);
        p.setAlgorithm(StageId::DefectLine, "destripe");
        p.setEnabled(StageId::ToneMap, true);
        p.setAlgorithm(StageId::ToneMap, "he");
        p.setEnabled(StageId::Detail, true);
        p.setAlgorithm(StageId::Detail, "guided");
    } else if (is("inspection")) {
        p.setEnabled(StageId::DefectPixel, true);
        p.setEnabled(StageId::Denoise, true);
        p.setAlgorithm(StageId::Denoise, "bilateral");
        p.setEnabled(StageId::ToneMap, true);
        p.setAlgorithm(StageId::ToneMap, "clahe");
    } else if (is("cleanup")) {
        p.setEnabled(StageId::DefectPixel, true);
        p.setEnabled(StageId::DefectLine, true);
        p.setAlgorithm(StageId::DefectLine, "replace");
    }
    return p;
}

} // namespace iat::isp
