#pragma once

#include "core/Histogram.h"
#include "core/Image2D.h"
#include "core/Processing.h"

#include <QFrame>
#include <QImage>

#include <limits>

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

// What a panel's export controls ask the tab to save.
enum class ExportKind {
    ImagePng,  // the image as displayed (current contrast), full resolution
    FrameMat,  // raw values of the image on display, plus Mean/Std/Noise
    StackMat,  // raw values of all frames (light / calibrated panels)
};

// One of the three panels in a session tab:
//   title and source information
//   image (zoom / pan / pixel read-out / right-click menu)
//   frame selector with ◀ ▶ buttons (light frames only)
//   histogram with contrast markers
//   Min / Max contrast sliders
//   Mean and Std of the displayed image (calculateImageStats) and the
//   temporal noise of the stack behind it (calculateImageNoise)
//   export buttons
//
// Light and Dark panels accept .mat files dropped onto them.
class ImagePanel : public QFrame {
    Q_OBJECT
public:
    explicit ImagePanel(PanelRole role, QWidget* parent = nullptr);

    PanelRole role() const { return m_role; }
    QString title() const;

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

    // Light panel: number of frames to choose from (0 hides the selector).
    // Resets the selection to the first frame.
    void setFrameCount(int count);
    int currentFrame() const;   // 0-based
    void setCurrentFrame(int index);

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
    void stepFrame(int delta);          // light panel: previous (-1) / next (+1) frame
    void copyImageToClipboard();
    void copyReportToClipboard();

signals:
    void fileDropped(const QString& path);
    void loadRequested();
    void currentFrameChanged(int index);
    void exportRequested(iat::ExportKind kind);
    void message(const QString& text);  // for the status bar

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    static QString matPathFrom(const QMimeData* mime);
    void setDropHighlight(bool on);
    void render();
    void updateStatsLabel();
    void updateFrameButtons();
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

    QLabel* m_title = nullptr;
    QLabel* m_source = nullptr;
    QToolButton* m_loadButton = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_placeholder = nullptr;
    ImageView* m_view = nullptr;
    QWidget* m_frameBar = nullptr;
    QToolButton* m_prevButton = nullptr;
    QToolButton* m_nextButton = nullptr;
    QSlider* m_frameSlider = nullptr;
    QSpinBox* m_frameSpin = nullptr;
    QLabel* m_frameTotal = nullptr;
    QCheckBox* m_logCheck = nullptr;
    QToolButton* m_autoButton = nullptr;
    QToolButton* m_fullButton = nullptr;
    HistogramWidget* m_histogram = nullptr;
    ContrastControl* m_contrast = nullptr;
    QLabel* m_pixelLabel = nullptr;
    QLabel* m_meanValue = nullptr;
    QLabel* m_stdValue = nullptr;
    QLabel* m_noiseValue = nullptr;
    QLabel* m_rangeLabel = nullptr;
    QToolButton* m_saveMatButton = nullptr;
    QToolButton* m_savePngButton = nullptr;
};

} // namespace iat
