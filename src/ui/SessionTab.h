#pragma once

#include "core/Image2D.h"
#include "core/ImageStack.h"
#include "ui/ImagePanel.h"
#include "ui/ImageView.h"

#include <QStringList>
#include <QWidget>

#include <limits>
#include <optional>

class QTimer;

namespace iat {

struct MatFileContents;
struct MatVariable;

// One tab: light frames, dark frames and the calibrated result.
//
// The light and dark panels each show one frame of their stack or the
// pixel-by-pixel mean over all frames (Single frame / Mean; by default the
// light panel shows one frame and the dark panel the mean). The calibrated
// panel shows
//
//   calibrated image = light image on display − dark image on display
//
// e.g. light(:, :, k) − mean(dark, 3) with the defaults, or
// light(:, :, k) − dark(:, :, j) when both panels show a single frame.
//
// A calibrated image can also be loaded from a file. Automatic calibration
// is then off (the panel shows a red indicator) until that image is removed
// again with restoreAutoCalibration().
//
// Each panel also shows calculateImageStats of its image and
// calculateImageNoise of the stack behind it (light, dark, light − dark
// image). The three image views share one zoom and pan.
//
// Files are read and written, and the means and noise values computed, on
// worker threads so the window stays responsive with large stacks.
class SessionTab : public QWidget {
    Q_OBJECT
public:
    explicit SessionTab(QWidget* parent = nullptr);

    // Loads one .mat file, or one or more TIFF / PNG / JPEG images, which
    // are stacked as frames in natural file-name order.
    void loadLight(const QStringList& paths);
    void loadDark(const QStringList& paths);
    void loadLight(const QString& path) { loadLight(QStringList{path}); }
    void loadDark(const QString& path) { loadDark(QStringList{path}); }
    // Loads a calibrated image or stack from one file (.mat or image) and
    // turns automatic calibration off.
    void loadCalibrated(const QString& path);
    // Removes a calibrated image loaded from a file and calculates
    // light − dark again.
    void restoreAutoCalibration();
    // True while the calibrated panel is calculated from light and dark.
    bool isAutoCalibration() const;

    void openLightDialog();
    void openDarkDialog();
    void openCalibratedDialog();

    // Asks for a file name, then saves. exportTo() saves without asking.
    void exportFrom(ImagePanel* panel, ExportKind kind);
    void exportTo(ImagePanel* panel, ExportKind kind, const QString& path);

    bool isBusy() const { return m_pending > 0; }

    ImagePanel* lightPanel() const { return m_lightPanel; }
    ImagePanel* darkPanel() const { return m_darkPanel; }
    ImagePanel* calibratedPanel() const { return m_calPanel; }

signals:
    void busyChanged(bool busy);
    void statusMessage(const QString& message);
    // Emitted when an export has finished (error is empty on success).
    void exportFinished(const QString& path, const QString& error);

private:
    enum class Target { Light = 0, Dark = 1, Calibrated = 2 };

    // A stack loaded into a panel, with its mean over frames and its noise.
    struct Source {
        quint64 id = 0;          // unique per load, to drop results of older loads
        ImageStack stack;
        QStringList paths;       // file(s) it was read from
        QString matlabClass;
        QString info;            // "file.mat · var · 6×8×4 uint16"
        QStringList notes;       // e.g. colour images converted to grey
        Image2D mean;            // mean(stack, 3), once statsReady
        double noise = std::numeric_limits<double>::quiet_NaN();
        bool statsReady = false;
        Image2D shown;           // image on display; null while waiting for the mean

        bool isNull() const { return stack.isNull(); }
        int frames() const { return stack.frameCount(); }
    };

    // What the calibrated noise was calculated for: one light stack and one
    // dark image (the mean, or frame darkFrame).
    struct NoiseKey {
        quint64 light = 0;
        quint64 dark = 0;
        int darkFrame = -1;
        bool operator==(const NoiseKey& o) const
        {
            return light == o.light && dark == o.dark && darkFrame == o.darkFrame;
        }
    };

    struct ReadResult;

    // Runs `work` on the thread pool and calls `done(result)` on the GUI thread.
    template <typename T, typename Work, typename Done>
    void runInBackground(Work work, Done done);

    Source& source(Target t);
    const Source& source(Target t) const;
    ImagePanel* panel(Target t) const;
    QString targetName(Target t) const;

    void startLoad(Target target, const QStringList& paths);
    void onFileRead(Target target, const ReadResult& result);
    std::optional<MatVariable> chooseVariable(Target target, const QString& path, const MatFileContents& contents);
    void setSource(Target target, Source source);
    void startStats(Target target);
    void loadFailed(Target target);
    void refreshPanel(Target target, bool resetContrast);
    QString panelInfo(Target target) const;
    void updateCalibrated(bool resetContrast);
    QString calibrationDescription() const;
    QString calibrationFormula(bool wholeStack) const;
    NoiseKey currentNoiseKey() const;
    void scheduleCalibratedNoise();
    void startCalibratedNoise();
    void addPending(int delta);
    void linkViews();
    QStringList openDialog(const QString& title, bool multiple);
    QString defaultExportName(ImagePanel* panel, ExportKind kind) const;

    ImagePanel* m_lightPanel = nullptr;
    ImagePanel* m_darkPanel = nullptr;
    ImagePanel* m_calPanel = nullptr;

    Source m_light;
    Source m_dark;
    Source m_cal;  // calibrated image loaded from a file (null: automatic calibration)
    quint64 m_nextSourceId = 0;

    // Incremented for every load; results from an older read are dropped.
    quint64 m_generation[3] = {0, 0, 0};
    bool m_reading[3] = {false, false, false};

    // Noise of the calibrated stack light − dark image, for one NoiseKey.
    NoiseKey m_calNoiseKey;
    double m_calNoise = std::numeric_limits<double>::quiet_NaN();
    bool m_calNoiseValid = false;
    QTimer* m_calNoiseTimer = nullptr;  // waits until the dark frame stops changing

    ViewState m_sharedView;  // zoom/pan shared by the three views
    int m_pending = 0;
};

} // namespace iat
