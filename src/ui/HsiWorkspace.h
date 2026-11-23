#pragma once

#include "core/Hsi.h"
#include "core/HsiAnalysis.h"
#include "core/HsiColour.h"
#include "io/HsiCubeIo.h"
#include "ui/ImageView.h"
#include "ui/CorrectionDialog.h"
#include "ui/ImportImageDialog.h"
#include "ui/ScorePlot.h"
#include "ui/SpectrumPlot.h"

#include <QFutureWatcher>
#include <QImage>
#include <QList>
#include <QRect>
#include <QWidget>

#include <array>
#include <memory>
#include <optional>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFrame;
class QGridLayout;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSlider;
class QSpinBox;
class QStackedWidget;
class QTabWidget;
class QTimer;
class QToolButton;

namespace iat {

class ImageView;

// The hyperspectral workspace of a tab (the third position of the layout
// switch): raw 4 × 4 mosaic → 16-band cube → processing → colour, spectra
// and analysis.
//
//   left    source (file, drop, or an image of another tab), mosaic layout
//           and extraction method, processing chain, colour reconstruction,
//           export
//   centre  the image: raw mosaic, one band, all bands, reconstructed
//           colour, false colour, principal components, a spectral index,
//           spectral similarity or classes; A/B before / after processing
//   right   spectra of the pixel under the mouse and of kept points and areas
//
// Extraction and processing run on a worker thread. The spectral response
// (reconstruction matrix) file is read when it is loaded and kept in memory
// only; nothing is copied next to the program or into exports.
class HsiWorkspace : public QWidget {
    Q_OBJECT
public:
    enum class ViewMode { Mosaic, Band, Gallery, Colour, FalseColour, Pca, Index, Similarity, Classes, Matches };
    static constexpr int kMaxPanes = 4;
    enum class Tool { Pan, Spectrum, WhiteReference, ColourReference, SimilarityReference };
    enum class CubeFormat { Envi, Mat, Tiff };

    // A kept point or area, in pixels of the unprocessed cube.
    struct Roi {
        QRect rect;
        QString name;
        QColor color;
    };

    explicit HsiWorkspace(QWidget* parent = nullptr);
    ~HsiWorkspace() override;

    void setImportProvider(ImportProvider provider) { m_importProvider = std::move(provider); }

    // ---- source
    // A raw mosaic (TIFF / PNG / JPEG / .mat with a 2-D array), a cube
    // (ENVI .hdr / .img, .mat with a 3-D array) or a spectral response
    // (.mat with lambda + QE, .csv). Returns false and reports why on failure.
    bool loadFile(const QString& path);
    void openFileDialog();
    void importFromWorkspace();
    bool importCandidate(const ImportCandidate& candidate);
    void setMosaic(const Image2D& raw, const QString& description);
    void setCube(const hsi::Cube& cube, const QString& description);
    const Image2D& mosaic() const { return m_mosaic; }
    QString sourceDescription() const { return m_sourceInfo; }

    // ---- extraction and processing
    hsi::ExtractOptions extractOptions() const;
    void setExtractOptions(const hsi::ExtractOptions& options);
    hsi::ProcessingSettings processingSettings() const;
    void setProcessingSettings(const hsi::ProcessingSettings& settings);
    const hsi::Cube& unprocessedCube() const { return m_cubeRaw; }
    const hsi::Cube& cube() const { return m_cube; }  // after processing

    // ---- correction (spectral response, spectral correction, reference frames)
    bool loadResponse(const QString& path);
    const CorrectionSettings& correctionSettings() const { return m_corr; }
    void applyCorrectionSettings(const CorrectionSettings& settings);
    // The spectral correction in use (invalid when off).
    const hsi::SpectralCorrection& spectralCorrection() const { return m_correction; }
    // The response of the bands of the cube: the spectral response, or C · A with correction.
    hsi::SpectralResponse effectiveResponse() const;
    // Dark (white = false) or white reference frame, at the raw mosaic's size.
    void setReferenceFrame(bool white, const Image2D& frame, const QString& name, const QString& path = QString());
    bool loadReferenceFrame(bool white, const QString& path);  // mean of all frames in the file
    void importReferenceFrame(bool white);
    void clearReferenceFrame(bool white);
    const hsi::ReferenceFrames& referenceFrames() const { return m_frames; }
    void openCorrectionSettings();
    bool saveSensorProfile(const QString& path);
    bool loadSensorProfile(const QString& path);
    // ---- score plot (PCA) and pixel selection
    void showScores();                       // the Scores tab, computed
    void setScoreComponents(int x, int y);   // 0-based components on the axes
    void selectScores(const QPolygonF& polygon);  // pixels whose scores lie inside
    void clearScoreSelection();
    void keepScoreSelection();  // its mean spectrum joins the list of spectra
    void removeKeptSelection(int index);
    int keptSelectionCount() const { return int(m_keptSelections.size()); }
    qsizetype selectionCount() const;
    const QVector<quint8>& selectionMask() const { return m_selectionMask; }
    ScorePlot* scorePlot() const { return m_scorePlot; }

    // The cube as extracted (or loaded), before spectral correction.
    const hsi::Cube& extractedCube() const { return m_cubeExtracted; }

    // ---- colour
    bool hasColour() const;
    QString responseName() const { return m_responseName; }
    hsi::ColourSettings colourSettings() const;
    void setColourSettings(const hsi::ColourSettings& settings);
    QVector<double> colourMatrix();  // 3 × bands, channel → linear sRGB; empty without a response

    // ---- view
    void setViewMode(ViewMode mode);
    ViewMode viewMode() const;
    void setBand(int band);
    int band() const;
    void setFalseColourBands(int r, int g, int b);
    void setIndexFormula(const QString& formula);
    QString indexError() const { return m_indexError; }
    void setTool(Tool tool);
    Tool tool() const { return m_tool; }
    void setAbCompare(bool on);
    bool abCompare() const;
    QImage displayedImage() const { return m_shown; }  // the first view's picture
    // One, two or four views side by side, each with its own view mode; zoom,
    // pan and a pointer mirroring the mouse follow in all of them.
    void setPaneCount(int count);
    int paneCount() const { return m_paneCount; }
    void setPaneMode(int pane, ViewMode mode);
    // The view that View:, the band slider and the false-colour bands act on
    // (click a view to make it active).
    void setActivePane(int pane);
    int activePane() const { return m_activePane; }
    // Refresh: make the cube again from the source with the current settings
    // (all) or draw one view again, recomputing what its mode keeps.
    void refreshAll();
    void refreshPane(int pane);
    // Reset: processing off, colour reconstruction and every view at their
    // defaults (all), or one view's mode, band, bands and zoom.
    void resetAll();
    void resetPane(int pane, bool render = true);
    ViewMode paneMode(int pane) const;
    ImageView* paneView(int pane) const { return m_panes[size_t(pane)].view; }
    QImage paneImage(int pane) const;
    // Pixels matched to each drawn area in the "Match to drawn areas" view.
    const hsi::AreaMatch& areaMatch() const { return m_match; }
    void setMatchAngle(double degrees);
    ImageView* view() const { return m_view; }
    SpectrumPlot* spectrumPlot() const { return m_plot; }

    // ---- analysis
    void addRoi(const QRect& rect, const QString& name = QString());
    const QList<Roi>& rois() const { return m_rois; }
    void clearRois();
    void removeRoi(int index);
    void setSimilarityReference(const QVector<double>& spectrum, const QString& name);
    void setSimilarityThreshold(double degrees);
    // Pixels within the threshold (Similarity view).
    qint64 similarPixelCount() const;
    void runClustering(int k);
    const hsi::Clustering& clustering() const { return m_clustering; }

    // ---- export
    bool exportCube(const QString& path, CubeFormat format, hsi::SampleType type, bool unprocessed = false,
                    QString* error = nullptr);
    bool exportView(const QString& path);
    bool exportSpectra(const QString& path, QString* error = nullptr);

    bool isBusy() const;
    // The "Processing…" indicator over the image is on show.
    bool busyIndicatorVisible() const;
    // Removes the white reference area and turns reflectance off.
    void clearWhiteReference();
    // Opens the mosaic layout settings dialog.
    void editMosaicLayout();

signals:
    void busyChanged(bool busy);
    void statusMessage(const QString& message);
    // The cube (after processing) was replaced.
    void cubeChanged();
    void activePaneChanged(int pane);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct JobResult;
    // Display stretch of a view, reused for its A/B "before" picture.
    struct Stretch {
        std::array<hsi::Range, 3> ranges{};
        double rgbScale = 0.0;
        bool valid = false;
    };
    // What an overlay drawn on a view stands for (to remove it by right-click).
    struct OverlayRef {
        enum Kind { Roi, White, ColourReference, SimilarityArea } kind = Roi;
        int index = 0;
    };
    struct Pane {
        QWidget* box = nullptr;
        QWidget* header = nullptr;
        QLabel* title = nullptr;
        QToolButton* refresh = nullptr;
        QToolButton* reset = nullptr;
        ImageView* view = nullptr;
        ViewMode viewMode = ViewMode::Band;
        int band = 0;
        std::array<int, 3> rgb{-1, -1, -1};  // false-colour bands; −1: not chosen yet
        QImage shown;
        QList<OverlayRef> overlayRefs;
    };

    QWidget* buildSourceGroup();
    QWidget* buildCorrectionGroup();
    void updateCorrection();
    void updateCorrectionStatus();
    void scheduleCorrection();
    QString responseDetails() const;
    QString framesDetails() const;
    QWidget* buildMosaicGroup();
    QWidget* buildProcessingGroup();
    QWidget* buildColourGroup();
    QWidget* buildExportGroup();
    QWidget* buildViewBar();
    QWidget* buildViewOptions();
    QWidget* buildSpectrumPanel();
    QWidget* buildScorePanel();
    void updateScores();
    void updateScoreGroups();
    void updateSelectionInfo();
    void tintSelection(QImage& img) const;
    QString unitsLabel(const QString& units) const;

    void scheduleUpdate(bool reextract);
    void startJob();
    void onJobFinished();
    void updateBusy();
    void cubeReplaced(bool sizeOrSourceChanged);
    void applyWavelengths();
    QVector<double> wavelengths() const;
    void updateLayoutLabel();
    void updateWhiteLabel();
    void showBusyIndicator(bool on);
    void updateBandControls();
    void invalidateColour();
    void invalidateAnalysis();
    void renderView();
    QImage renderMode(ViewMode mode, const hsi::Cube& cube, bool forCompare, Stretch* stretch);
    void fillViewModes(QComboBox* combo);
    bool anyPaneShows(ViewMode mode) const;
    void updateAbButton();
    void updateOverlays();
    void removeOverlay(int pane, int index);
    double paneScale(int pane) const;
    void onPaneViewChanged(int pane);
    void applySharedView(int pane);
    bool toCubePixel(int pane, int x, int y, int& cx, int& cy) const;
    void updatePointers(int sourcePane, int cx, int cy);
    QRect toCube(const QRect& baseRect, const hsi::Cube& cube) const;
    void updateSpectra();
    void updateCubeInfo();
    void onHover(int pane, int x, int y);
    void onPixelClicked(int pane, int x, int y);
    void onRegionSelected(int pane, const QRect& displayRect);
    void handleSelection(const QRect& baseRect, bool isPoint);
    double displayScale(int pane) const;  // displayed-image pixels per unprocessed-cube pixel
    double processedScale() const;    // processed-cube pixels per unprocessed-cube pixel
    QRect toProcessed(const QRect& baseRect) const;
    void exportCubeDialog();
    void exportViewDialog();
    void exportSpectraDialog();
    void loadResponseDialog();
    QString lastDir() const;
    void rememberDir(const QString& path);
    void report(const QString& message);
    QString baseName() const;

    // ---- data
    ImportProvider m_importProvider;
    Image2D m_mosaic;
    QString m_sourceInfo;
    QString m_sourceName;              // for default export names
    hsi::Cube m_cubeExtracted;         // extracted (or loaded), before spectral correction
    hsi::Cube m_cubeRaw;               // after spectral correction, before processing
    hsi::Cube m_cube;                  // after processing
    std::optional<hsi::MosaicLayout> m_cubeLayout;  // layout the cube was extracted with
    bool m_fromMosaic = false;

    hsi::SpectralResponse m_response;
    QVector<double> m_directRgb;       // a 3 × B matrix loaded as it is
    QString m_responseName;
    QString m_responsePath;
    QString m_responseNotes;
    CorrectionSettings m_corr;
    hsi::SpectralCorrection m_correction;  // in use; invalid when off
    hsi::ReferenceFrames m_frames;
    QString m_darkName, m_whiteName, m_darkPath, m_whitePath;
    bool m_needCorrect = false;
    QLabel* m_responseStatus = nullptr;
    QCheckBox* m_correctionCheck = nullptr;
    QLabel* m_darkLabel = nullptr;
    QLabel* m_whiteLabel2 = nullptr;
    QToolButton* m_clearDark = nullptr;
    QToolButton* m_clearWhiteFrame = nullptr;
    QVector<double> m_xyz;             // cached 3 × B XYZ matrix
    QVector<double> m_rgb;             // cached 3 × B sRGB matrix (for m_cube)
    bool m_colourDirty = true;

    QRect m_whiteRef;                  // unprocessed-cube pixels
    QRect m_colourRef;
    QRect m_similarityArea;
    QVector<double> m_similarityRef;
    QString m_similarityName;
    QVector<quint8> m_similarMask;
    hsi::Pca m_pca;
    hsi::Clustering m_clustering;
    std::shared_ptr<hsi::BandMath> m_index;
    QString m_indexError;
    Image2D m_indexImage;
    QList<Roi> m_rois;
    int m_roiCounter = 0;
    QVector<double> m_hoverSpectrum;
    QString m_hoverName;
    QImage m_shown;
    Tool m_tool = Tool::Spectrum;
    Tool m_previousTool = Tool::Spectrum;
    std::array<Pane, kMaxPanes> m_panes;
    int m_activePane = 0;
    QTabWidget* m_rightTabs = nullptr;
    ScorePlot* m_scorePlot = nullptr;
    SpectrumPlot* m_loadingsPlot = nullptr;
    QComboBox* m_scoreX = nullptr;
    QComboBox* m_scoreY = nullptr;
    QLabel* m_scoreInfo = nullptr;
    QPushButton* m_clearSelection = nullptr;
    QPushButton* m_keepSelection = nullptr;
    Image2D m_scoreXImage, m_scoreYImage;
    int m_scoreXIndex = -1, m_scoreYIndex = -1;
    QVector<quint8> m_selectionMask;  // per pixel of m_cube; empty: none
    QList<SpectrumPlot::Series> m_keptSelections;
    QCheckBox* m_absorbance = nullptr;
    QComboBox* m_normalisation = nullptr;
    QComboBox* m_derivative = nullptr;
    QSpinBox* m_sgWindow = nullptr;
    QSpinBox* m_sgPolynomial = nullptr;
    int m_renderPane = -1;  // the pane renderMode() is drawing, −1: the active one
    int renderBand() const;
    std::array<int, 3> paneRgbBands() const;
    void updatePaneTitles();
    void updateGridStretch();
    int m_paneCount = 1;
    QWidget* m_viewGrid = nullptr;
    QGridLayout* m_gridLayout = nullptr;
    QComboBox* m_paneCountCombo = nullptr;
    ViewState m_sharedView;  // zoom in display pixels per processed-cube pixel, centre in cube pixels
    bool m_haveSharedView = false;
    bool m_syncing = false;
    hsi::AreaMatch m_match;
    QDoubleSpinBox* m_matchAngle = nullptr;
    QCheckBox* m_matchShade = nullptr;
    QComboBox* m_matchBackground = nullptr;
    QLabel* m_matchLabel = nullptr;

    // ---- background work
    QTimer* m_updateTimer = nullptr;
    QFutureWatcher<JobResult>* m_watcher = nullptr;
    QFutureWatcher<hsi::Clustering>* m_clusterWatcher = nullptr;
    bool m_needExtract = false;
    bool m_running = false;
    bool m_rerun = false;
    bool m_busyReported = false;
    bool m_loadingSettings = false;

    // ---- widgets
    QLabel* m_sourceLabel = nullptr;
    hsi::MosaicLayout m_layout = hsi::MosaicLayout::defaultLayout();
    QLabel* m_layoutLabel = nullptr;
    QFrame* m_busyOverlay = nullptr;
    QTimer* m_busyDelay = nullptr;
    QPushButton* m_clearWhite = nullptr;
    QDoubleSpinBox* m_blackLevel = nullptr;
    QSpinBox* m_bitDepth = nullptr;
    QCheckBox* m_clipNegative = nullptr;
    QComboBox* m_method = nullptr;
    QLabel* m_cubeInfo = nullptr;
    QGroupBox* m_mosaicGroup = nullptr;

    QCheckBox* m_reflectance = nullptr;
    QDoubleSpinBox* m_whiteValue = nullptr;
    QLabel* m_whiteLabel = nullptr;
    QCheckBox* m_mnf = nullptr;
    QSpinBox* m_mnfKeep = nullptr;
    QCheckBox* m_guided = nullptr;
    QSpinBox* m_guidedRadius = nullptr;
    QDoubleSpinBox* m_guidedEps = nullptr;
    QCheckBox* m_antiAlias = nullptr;
    QDoubleSpinBox* m_antiAliasSigma = nullptr;
    QComboBox* m_upscale = nullptr;
    QComboBox* m_interpolation = nullptr;

    QDoubleSpinBox* m_sigma = nullptr;
    QDoubleSpinBox* m_alpha = nullptr;
    QComboBox* m_wb = nullptr;
    QDoubleSpinBox* m_exposure = nullptr;
    QPushButton* m_pickColourRef = nullptr;

    QComboBox* m_exportFormat = nullptr;
    QComboBox* m_exportType = nullptr;
    QCheckBox* m_exportUnprocessed = nullptr;

    QComboBox* m_viewMode = nullptr;
    QComboBox* m_toolCombo = nullptr;
    QToolButton* m_abButton = nullptr;
    QStackedWidget* m_centre = nullptr;
    QLabel* m_dropHint = nullptr;
    ImageView* m_view = nullptr;
    QLabel* m_readout = nullptr;
    QStackedWidget* m_viewOptions = nullptr;
    QSlider* m_bandSlider = nullptr;
    QLabel* m_bandLabel = nullptr;
    std::array<QComboBox*, 3> m_rgbBands{};
    QLabel* m_pcaLabel = nullptr;
    QComboBox* m_indexPreset = nullptr;
    QLineEdit* m_indexFormula = nullptr;
    QComboBox* m_colormap = nullptr;
    QLabel* m_indexLabel = nullptr;
    QDoubleSpinBox* m_simThreshold = nullptr;
    QComboBox* m_simDisplay = nullptr;
    QLabel* m_simLabel = nullptr;
    QSpinBox* m_clusterK = nullptr;
    QCheckBox* m_clusterShape = nullptr;
    QLabel* m_clusterLabel = nullptr;

    SpectrumPlot* m_plot = nullptr;
    QCheckBox* m_plotWavelength = nullptr;
    QCheckBox* m_plotNormalized = nullptr;
    QCheckBox* m_plotClasses = nullptr;
    QListWidget* m_roiList = nullptr;
};

} // namespace iat
