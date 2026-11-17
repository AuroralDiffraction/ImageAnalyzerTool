#pragma once

#include "core/Histogram.h"
#include "core/Image2D.h"
#include "core/Isp.h"
#include "core/Processing.h"

#include <QFrame>
#include <QFutureWatcher>
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
class PipelineDialog;

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
//   title, Single frame / Mean toggle (light and dark), Processing…, A/B
//   and Load… buttons
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
//
// Image processing: every panel has its own processing pipeline
// (isp::Pipeline: defect correction, denoising, tone mapping, sharpening, …),
// edited in a PipelineDialog opened with the Processing… button. While a
// stage is on, the panel shows the processed image: image() is the result,
// rawImage() what the tab gave the panel, and the histogram, contrast range,
// Mean and Std describe the processed image. The pipeline runs on a worker
// thread; the previous picture stays until the new one is ready. Other
// panels are not affected: the tab calculates the calibrated image from the
// unprocessed light and dark images.
//
// A/B: with the A/B button on, a divider can be dragged across a processed
// image; left of it the view shows the unprocessed image (A), right of it
// the processed one (B). See ImageView::setCompareImage.
//
// In a tab's single image mode the light panel is the only one on show and
// is presented as a plain "Image" panel (setSingleImage): same controls and
// calculations, but its title and texts no longer speak of light frames.
class ImagePanel : public QFrame {
    Q_OBJECT
public:
    explicit ImagePanel(PanelRole role, QWidget* parent = nullptr);

    PanelRole role() const { return m_role; }
    QString title() const;

    // Light panel only: present it as the "Image" panel of single image mode.
    void setSingleImage(bool on);
    bool isSingleImage() const { return m_single; }

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
    // The image on display: the processed image while the pipeline is on,
    // otherwise the same as rawImage().
    const Image2D& image() const { return m_image; }
    // The image as given to setImage(), before any processing.
    const Image2D& rawImage() const { return m_raw; }

    // Image processing pipeline of this panel. setPipeline() processes the
    // current image again (in the background) if the settings changed.
    const isp::Pipeline& pipeline() const { return m_pipeline; }
    void setPipeline(const isp::Pipeline& pipeline);
    // True if processing is on (a stage is enabled and "Show original" is off).
    bool processingActive() const { return m_pipeline.isActive() && !m_showOriginal; }
    // True if image() is a processed image.
    bool isProcessed() const { return m_processed; }
    // True while the pipeline is running for an image that is not shown yet.
    bool isProcessing() const { return m_processBusy; }
    // "Show original": the unprocessed image, with the settings kept.
    bool showOriginal() const { return m_showOriginal; }
    void setShowOriginal(bool on);
    // A/B comparison: a divider across the image, the unprocessed image to
    // its left and the processed image to its right. It shows whenever the
    // image on display is a processed one.
    bool abCompare() const { return m_abCompare; }
    void setAbCompare(bool on);  // emits abCompareChanged if it changes
    // The enabled stages of the image on display, e.g. "Denoise: Bilateral
    // filter; Tone mapping: CLAHE" (English), or empty if it is unprocessed.
    QString processingSummary() const;
    // The settings dialog (created on first use; not shown by this call).
    PipelineDialog* pipelineDialog();
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
    void openPipelineDialog();          // the Processing… button

signals:
    // Files dropped onto the panel, already checked with selectFiles().
    void filesDropped(const QStringList& paths);
    void loadRequested();
    void currentFrameChanged(int index);
    void frameModeChanged(iat::FrameMode mode);
    // The "Auto calibration off" indicator was clicked.
    void autoCalibrationRequested();
    void exportRequested(iat::ExportKind kind);
    // The pipeline started (true) or finished (false) working in the background.
    void processingChanged(bool busy);
    void abCompareChanged(bool on);
    void message(const QString& text);  // for the status bar

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QStringList droppedPaths(const QMimeData* mime, QString* error) const;
    void setDropHighlight(bool on);
    QColor accent() const;
    void updateModeTooltips();
    void updateFrameControls();
    void updateStatTooltips();
    void render();
    void display(const Image2D& image, bool resetContrast, bool displayReferred);
    void reprocess();
    void startProcessing(bool resetContrast);
    void launchProcessing();
    void onProcessingFinished();
    void setProcessBusy(bool busy);
    void updateProcessButton();
    void updateHeaderButtons();
    void updateCompareImage();
    void updateStatsLabel();
    void showPixel(int x, int y);
    void setControlsEnabled(bool on);
    void showContextMenu(const QPoint& globalPos);
    QString noiseText() const;

    PanelRole m_role;
    bool m_single = false;
    Image2D m_raw;    // as given by the tab
    Image2D m_image;  // on display (processed, or the same as m_raw)
    Image2D m_shownRaw;  // the unprocessed image that m_image was made from

    // Image processing.
    isp::Pipeline m_pipeline;
    isp::Pipeline m_shownPipeline;     // the settings of the image on display (if m_processed)
    bool m_showOriginal = false;
    bool m_abCompare = false;          // A/B divider wanted (shown while m_processed)
    const double* m_beforeRangeFor = nullptr;  // the image the A side's display range was calculated for
    double m_beforeLow = 0.0;
    double m_beforeHigh = 1.0;
    bool m_processed = false;          // m_image is a processed image
    bool m_displayReferred = false;    // m_image holds display values 0..255
    bool m_processBusy = false;        // a worker is running
    bool m_processAgain = false;       // the input or settings changed meanwhile
    bool m_processReset = false;       // reset the contrast when the result is shown
    quint64 m_processGeneration = 0;   // of the newest request
    quint64 m_runningGeneration = 0;   // of the worker
    Image2D m_runningRaw;              // what the worker is processing
    isp::Pipeline m_runningPipeline;
    QFutureWatcher<isp::Result>* m_processWatcher = nullptr;
    PipelineDialog* m_pipelineDialog = nullptr;
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
    bool m_dropActive = false;  // files are being dragged over the panel

    QFrame* m_accent = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_source = nullptr;
    QToolButton* m_loadButton = nullptr;
    QToolButton* m_processButton = nullptr;
    QToolButton* m_abButton = nullptr;
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
