#pragma once

#include "core/Image2D.h"

#include <QString>
#include <QStringList>
#include <QVector>

#include <vector>

// A small image signal processing (ISP) pipeline for monochrome sensor data.
//
// The pipeline is a fixed chain of stages, in the order a camera ISP applies
// them to raw data:
//
//   black level -> defect pixels -> defect rows/columns -> digital gain
//   -> denoise -> dynamic range compression -> tone mapping
//   -> detail enhancement -> sharpening -> output quantisation
//
// The colour stages of a camera ISP (demosaic, white balance, colour
// matrix) are left out: the data is monochrome.
//
// Every stage can be bypassed and offers a choice of algorithms, each with
// its own parameters. The stages, algorithms and parameters are described by
// tables (stages()), so the user interface is built from the same tables and
// a new algorithm only has to be added here.
//
// Units: the stages before tone mapping work in the input's own units (DN)
// and keep them. Tone mapping converts to display values 0..255 (floating
// point, not yet rounded); the stages after it work on those. Output
// quantisation rounds to 8, 6 or 4 bits, with optional dither.
//
// Everything is done in double precision on one image. The functions are
// pure (image in, image out) and have no GUI dependency.
//
// The names and descriptions in the tables are English and marked for
// translation in the context "iat::isp"; translate them with text().
namespace iat::isp {

enum class ParamKind {
    Real,     // floating-point value from min to max
    Integer,  // whole number from min to max
    Choice,   // index into `choices`
};

struct ParamSpec {
    const char* id = "";
    const char* label = "";  // translatable
    ParamKind kind = ParamKind::Real;
    double min = 0.0;
    double max = 1.0;
    double step = 0.01;
    double def = 0.0;
    bool logScale = false;   // the slider moves in equal ratios
    const char* unit = "";   // translatable, may be empty
    std::vector<const char*> choices;  // translatable, for ParamKind::Choice
};

struct AlgorithmSpec {
    const char* id = "";
    const char* name = "";         // translatable
    const char* description = "";  // translatable
    std::vector<ParamSpec> params;
};

enum class StageId {
    BlackLevel,
    DefectPixel,
    DefectLine,
    Gain,
    Denoise,
    DynamicRange,
    ToneMap,
    Detail,
    Sharpen,
    Output,
};

struct StageSpec {
    StageId id = StageId::BlackLevel;
    const char* key = "";          // stable name, e.g. "denoise"
    const char* name = "";         // translatable
    const char* shortName = "";    // translatable; one or two words, for the node in the chain
    const char* description = "";  // translatable
    std::vector<AlgorithmSpec> algorithms;  // at least one
};

// The stages in processing order.
const std::vector<StageSpec>& stages();
int stageIndex(StageId id);
const StageSpec& stage(StageId id);

// Translation of a text from the tables (context "iat::isp").
QString text(const char* source);

// The settings of one stage: bypassed or not, which algorithm, and the
// parameter values of every algorithm (so switching algorithm and back
// keeps the values).
struct StageConfig {
    bool enabled = false;
    int algorithm = 0;
    QVector<QVector<double>> values;  // [algorithm][parameter]

    bool operator==(const StageConfig& o) const
    {
        return enabled == o.enabled && algorithm == o.algorithm && values == o.values;
    }
    bool operator!=(const StageConfig& o) const { return !(*this == o); }
};

class Pipeline {
public:
    // Every stage bypassed, first algorithm, default parameter values.
    Pipeline();

    const StageConfig& config(StageId id) const { return m_stages[stageIndex(id)]; }
    StageConfig& config(StageId id) { return m_stages[stageIndex(id)]; }

    bool isEnabled(StageId id) const;
    void setEnabled(StageId id, bool on);
    // Selects an algorithm by its id. Returns false if there is none.
    bool setAlgorithm(StageId id, const char* algorithmId);
    const AlgorithmSpec* algorithm(StageId id) const;  // the selected one, or null

    // Parameter of the selected algorithm of a stage, by id. value() returns
    // NaN and setValue() false for an unknown id. Values are clamped to the
    // parameter's range.
    double value(StageId id, const char* paramId) const;
    bool setValue(StageId id, const char* paramId, double value);

    void resetStage(StageId id);  // default values, bypassed

    // True if at least one stage is enabled.
    bool isActive() const;
    // True if the result is in display values (0..255): tone mapping is on.
    bool isDisplayReferred() const { return isEnabled(StageId::ToneMap); }
    // True if both pipelines give results on the same value scale, so a
    // display range chosen for one still makes sense for the other.
    bool sameScaleAs(const Pipeline& other) const;

    // The enabled stages and their algorithms, e.g.
    // "Denoise: Bilateral filter; Tone mapping: CLAHE". English, for
    // exported files (translated = false), or in the interface language.
    QString summary(bool translated) const;

    bool operator==(const Pipeline& o) const { return m_stages == o.m_stages; }
    bool operator!=(const Pipeline& o) const { return !(*this == o); }

private:
    QVector<StageConfig> m_stages;
};

struct Result {
    Image2D image;
    // The image holds display values 0..255 (tone mapping was applied).
    bool displayReferred = false;
    // One entry per stage (same order as stages()): what the stage did, in
    // the interface language, e.g. "12 pixels corrected". Empty for
    // bypassed stages.
    QStringList notes;
    double milliseconds = 0.0;
};

// Runs the enabled stages on `input`. NaN and Inf pixels are left as they
// are (they are ignored by the filters and put back at the end). With no
// stage enabled the result is the input itself.
Result run(const Image2D& input, const Pipeline& pipeline);

// Robust estimate of the standard deviation of the pixel noise, from the
// median absolute value of a noise-sensitive 3 × 3 difference (so edges and
// texture have little influence). Used to scale the thresholds of the
// defect, denoise and sharpening stages.
double estimateNoise(const Image2D& image);

// Ready-made settings for common situations.
struct Preset {
    const char* id = "";
    const char* name = "";         // translatable
    const char* description = "";  // translatable
};
const std::vector<Preset>& presets();
Pipeline presetPipeline(const char* presetId);

} // namespace iat::isp
