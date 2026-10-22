#pragma once

#include "core/Histogram.h"
#include "core/Image2D.h"
#include "core/Processing.h"

#include <QFrame>
#include <QImage>

#include <limits>

class QButtonGroup;
class QCheckBox;
class QLabel;
class QMimeData;
class QSlider;
class QSpinBox;
class QStackedWidget;
class QToolButton;
class QWidget;

namespace iat {

class ContrastControl;
class HistogramWidget;
class ImageView;

enum class PanelRole { Light, Dark, Calibrated };

// What the light and dark panels show from their stack.
enum class FrameMode {
    Single,  // one frame, chosen with the frame selector
    Mean,    // pixel-by-pixel mean over all frames (MATLAB: mean(stack, 3))
};

// What a panel's export controls ask the tab to save.
enum class ExportKind {
    ImagePng,  // the image as displayed (current contrast), full resolution
    FrameMat,  // raw values of the image on display, plus Mean/Std/Noise
    StackMat,  // raw values of all frames of the stack behind the panel
};

// One of the three panels in a session tab:
//   title, Single frame / Mean toggle (light and dark), Load… button
//   source information
//   image (zoom / pan / pixel read-out / right-click menu)
//   frame selector with ◀ ▶ buttons (light and dark frames, and a stack
//   loaded into the calibrated panel)
//   histogram with contrast markers
//   Min / Max contrast sliders
//   Mean and Std of the displayed image (calculateImageStats) and the
//   temporal noise of the stack behind it (calculateImageNoise)
//   export buttons
//
// Every panel accepts files dropped onto it: one .mat file, or TIFF / PNG /
// JPEG images (several at once in the light and dark panels).
//
// The calibrated panel shows a red "Auto calibration off" indicator while it
// holds an image loaded from a file; clicking it asks to go back to
// automatic calibration.
class ImagePanel : public QFrame {
    Q_OBJECT
public:
    explicit ImagePanel(PanelRole role, QWidget* parent = nullptr);

    PanelRole role() const { return m_role; }
    QString title() const;

    // The same width for every panel, so a tab's three images start out
    // the same size and line up (the controls make the minimum widths differ).
    QSize sizeHint() const override;

    // Shows an image. With resetContrast the displayed range is set
    // automatically (0.5–99.5 % percentiles); otherwise the current Min/Max
    // are kept (clamped to the new image's range).
    void setImage(const Image2D& image, bool resetContrast);
    // Removes the image and shows a message in its place.
    void showPlaceholder(const QString& text);
    void showBusy(const QString& text);

    // Line under the title, e.g. "file.mat · all_pixel · 1028×1408×20".
    void setSourceInfo(const QString& text, const QString& tooltip = QString());
    QString sourceInfo() const;

    // Number of frames to choose from (0 hides the selector). Resets the
    // selection to the first frame.
    void setFrameCount(int count);
    int frameCount() const { return m_frameCount; }
    int currentFrame() const;   // 0-based
    void setCurrentFrame(int index);

    // Light and dark panels: one frame or the mean over all frames. The
    // frame selector is disabled in Mean mode. Defaults: light Single,
    // dark Mean. The calibrated panel is always Single.
    FrameMode frameMode() const { return m_mode; }
    void setFrameMode(FrameMode mode);  // emits frameModeChanged if it changes

    // Calibrated panel: shows or hides the red "Auto calibration off" indicator.
    void setAutoCalibrationOff(bool off);
    bool isAutoCalibrationOff() const { return m_autoOff; }

    // Temporal noise (calculateImageNoise) of the stack behind this panel.
    // It is computed in the background, so the panel can show "…" until the
    // value arrives. The value is kept when the displayed frame changes.
    void setNoisePending();
    void setNoise(double noise, int frameCount);
    void clearNoise();

    bool hasImage() const { return !m_image.isNull(); }
    const Image2D& image() const { return m_image; }
    const ImageStats& stats() const { return m_stats; }
    // calculateImageStats of the displayed image.
    const processing::MeanStd& meanStd() const { return m_meanStd; }
    double noise() const { return m_noise; }
    int noiseFrameCount() const { return m_noiseFrames; }
    double displayLow() const { return m_low; }
    double displayHigh() const { return m_high; }

    ImageView* view() const { return m_view; }

    // The image as displayed: 8-bit grey, full resolution, current contrast.
    QImage displayedImage() const { return m_rendered; }
    // Figure for "Copy image with histogram and statistics": title, the
    // displayed image, the histogram exactly as shown in the panel (scaled to
    // the image width) and Mean / Std / Noise.
    QImage renderReport() const;

public slots:
    void setDisplayRange(double low, double high);
    void autoContrast();
    void fullRangeContrast();
    void stepFrame(int delta);          // previous (-1) / next (+1) frame
    void copyImageToClipboard();
    void copyReportToClipboard();

signals:
    // Files dropped onto the panel, already checked with selectFiles().
    void filesDropped(const QStringList& paths);
    void loadRequested();
    void currentFrameChanged(int index);
    void frameModeChanged(iat::FrameMode mode);
    // The "Auto calibration off" indicator was clicked.
    void autoCalibrationRequested();
    void exportRequested(iat::ExportKind kind);
    void message(const QString& text);  // for the status bar

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QStringList droppedPaths(const QMimeData* mime, QString* error) const;
    void setDropHighlight(bool on);
    void updateFrameControls();
    void updateStatTooltips();
    void render();
    void updateStatsLabel();
    void showPixel(int x, int y);
    void setControlsEnabled(bool on);
    void showContextMenu(const QPoint& globalPos);
    QString noiseText() const;

    PanelRole m_role;
    Image2D m_image;
    QImage m_rendered;
    ImageStats m_stats;
    processing::MeanStd m_meanStd;
    double m_noise = std::numeric_limits<double>::quiet_NaN();
    int m_noiseFrames = 0;
    bool m_noisePending = false;
    Histogram m_hist;
    double m_low = 0.0;
    double m_high = 1.0;
    bool m_rangeValid = false;
    QString m_defaultPlaceholder;
    FrameMode m_mode = FrameMode::Single;
    int m_frameCount = 0;
    bool m_autoOff = false;

    QLabel* m_title = nullptr;
    QLabel* m_source = nullptr;
    QToolButton* m_loadButton = nullptr;
    QToolButton* m_singleButton = nullptr;
    QToolButton* m_meanButton = nullptr;
    QButtonGroup* m_modeGroup = nullptr;
    QToolButton* m_autoOffButton = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_placeholder = nullptr;
    ImageView* m_view = nullptr;
    QWidget* m_frameBar = nullptr;
    QToolButton* m_prevButton = nullptr;
    QToolButton* m_nextButton = nullptr;
    QSlider* m_frameSlider = nullptr;
    QSpinBox* m_frameSpin = nullptr;
    QLabel* m_frameLabel = nullptr;
    QLabel* m_frameTotal = nullptr;
    QCheckBox* m_logCheck = nullptr;
    QToolButton* m_autoButton = nullptr;
    QToolButton* m_fullButton = nullptr;
    HistogramWidget* m_histogram = nullptr;
    ContrastControl* m_contrast = nullptr;
    QLabel* m_pixelLabel = nullptr;
    QLabel* m_meanValue = nullptr;
    QLabel* m_stdValue = nullptr;
    QLabel* m_meanCaption = nullptr;
    QLabel* m_stdCaption = nullptr;
    QLabel* m_noiseValue = nullptr;
    QLabel* m_rangeLabel = nullptr;
    QToolButton* m_saveMatButton = nullptr;
    QToolButton* m_savePngButton = nullptr;
};

} // namespace iat
