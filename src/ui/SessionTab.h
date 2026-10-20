#pragma once

#include "core/Image2D.h"
#include "core/ImageStack.h"
#include "ui/ImagePanel.h"
#include "ui/ImageView.h"

#include <QWidget>

#include <limits>
#include <optional>

namespace iat {

struct MatFileContents;
struct MatVariable;

// One tab: light frames, dark frames and the calibrated result.
//
//   dark frame       = mean over Z of the dark stack   (MATLAB: mean(dark, 3))
//   calibrated frame = light(:, :, k) - dark frame
//
// Each panel also shows calculateImageStats of its image and
// calculateImageNoise of the stack behind it (light, dark, light - dark).
// The three image views share one zoom and pan.
//
// Files are read and written, and the dark mean and noise values computed,
// on worker threads so the window stays responsive with large stacks.
class SessionTab : public QWidget {
    Q_OBJECT
public:
    explicit SessionTab(QWidget* parent = nullptr);

    void loadLight(const QString& path);
    void loadDark(const QString& path);
    void openLightDialog();
    void openDarkDialog();

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
    enum class Target { Light, Dark };

    // Runs `work` on the thread pool and calls `done(result)` on the GUI thread.
    template <typename T, typename Work, typename Done>
    void runInBackground(Work work, Done done);

    void startRead(Target target, const QString& path);
    void onFileRead(Target target, const QString& path, const MatFileContents& contents);
    std::optional<MatVariable> chooseVariable(Target target, const QString& path, const MatFileContents& contents);
    void setLightStack(const QString& path, const MatVariable& variable);
    void startDarkMean(const QString& path, const MatVariable& variable);
    void startLightNoise();
    void startCalibratedNoise();
    void restorePanel(Target target);
    void showLightFrame(int index, bool resetContrast);
    void updateCalibrated(bool resetContrast);
    void addPending(int delta);
    void linkViews();
    QString openDialog(const QString& title);
    QString defaultExportName(ImagePanel* panel, ExportKind kind) const;
    quint64& generation(Target target) { return target == Target::Light ? m_lightGeneration : m_darkGeneration; }

    ImagePanel* m_lightPanel = nullptr;
    ImagePanel* m_darkPanel = nullptr;
    ImagePanel* m_calPanel = nullptr;

    ImageStack m_lightStack;
    Image2D m_lightFrame;
    QString m_lightInfo;
    QString m_lightPath;
    QString m_lightClass;  // MATLAB class of the light variable
    double m_lightNoise = std::numeric_limits<double>::quiet_NaN();
    Image2D m_darkMean;
    QString m_darkPath;
    int m_darkFrameCount = 0;
    double m_darkNoise = std::numeric_limits<double>::quiet_NaN();

    ViewState m_sharedView;  // zoom/pan shared by the three views

    // Incremented for every load; results from an older load are dropped.
    quint64 m_lightGeneration = 0;
    quint64 m_darkGeneration = 0;
    int m_pending = 0;
};

} // namespace iat
