#include "ui/HsiWorkspace.h"

#include "io/ImageFileReader.h"
#include "io/MatFileReader.h"
#include "ui/ImageView.h"
#include "ui/MosaicLayoutDialog.h"
#include "ui/Icons.h"
#include "ui/ScorePlot.h"
#include "core/Processing.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QStyle>
#include <QMessageBox>
#include <QTabWidget>
#include <QMimeData>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <cmath>
#include <iterator>

namespace iat {

namespace {
const std::array<HsiWorkspace::ViewMode, 4> kDefaultPaneModes = {HsiWorkspace::ViewMode::Band, HsiWorkspace::ViewMode::Colour,
                                                               HsiWorkspace::ViewMode::FalseColour, HsiWorkspace::ViewMode::Pca};
const QColor kSelectionColour(255, 0, 200);  // pixels selected on the score plot
}

using namespace hsi;

namespace {

constexpr double kRadToDeg = 57.29577951308232;

QString sizeText(int rows, int cols) { return QStringLiteral("%1 × %2").arg(rows).arg(cols); }

QColor roiColour(int index)
{
    static const QColor colours[] = {QColor(0xe6, 0x39, 0x46), QColor(0x2a, 0x9d, 0x8f), QColor(0xf4, 0xa2, 0x61),
                                     QColor(0x45, 0x7b, 0x9d), QColor(0x9b, 0x5d, 0xe5), QColor(0x8a, 0xc9, 0x26),
                                     QColor(0xff, 0x70, 0xa6), QColor(0x00, 0xb4, 0xd8)};
    return colours[index % int(std::size(colours))];
}

QDoubleSpinBox* doubleSpin(QWidget* parent, double lo, double hi, double value, int decimals, double step)
{
    auto* s = new QDoubleSpinBox(parent);
    s->setRange(lo, hi);
    s->setDecimals(decimals);
    s->setSingleStep(step);
    s->setValue(value);
    return s;
}

QSpinBox* intSpin(QWidget* parent, int lo, int hi, int value)
{
    auto* s = new QSpinBox(parent);
    s->setRange(lo, hi);
    s->setValue(value);
    return s;
}

QLabel* hint(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text, parent);
    l->setWordWrap(true);
    l->setForegroundRole(QPalette::PlaceholderText);
    return l;
}

} // namespace

struct HsiWorkspace::JobResult {
    Cube extractedCube;  // extracted (or loaded), before spectral correction
    Cube raw;            // after spectral correction, before processing
    Cube processed;
    QString error;
    QStringList notes;
    bool extracted = false;
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

HsiWorkspace::HsiWorkspace(QWidget* parent)
    : QWidget(parent)
{
    setAcceptDrops(true);

    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(120);
    connect(m_updateTimer, &QTimer::timeout, this, [this] {
        startJob();
        updateBusy();
    });
    m_watcher = new QFutureWatcher<JobResult>(this);
    connect(m_watcher, &QFutureWatcherBase::finished, this, &HsiWorkspace::onJobFinished);
    m_clusterWatcher = new QFutureWatcher<Clustering>(this);
    connect(m_clusterWatcher, &QFutureWatcherBase::finished, this, [this] {
        const Clustering c = m_clusterWatcher->result();
        if (c.isValid() && c.labels.size() == m_cube.planeSize()) {
            m_clustering = c;
            QStringList sizes;
            for (int i = 0; i < c.k; ++i)
                sizes << QStringLiteral("%1: %2 %").arg(i + 1).arg(100.0 * double(c.sizes[i]) / double(m_cube.planeSize()), 0, 'f', 1);
            m_clusterLabel->setText(tr("Classes (share of pixels): %1").arg(sizes.join(QStringLiteral(", "))));
            renderView();
            updateSpectra();
        } else {
            report(tr("Clustering did not give a result."));
        }
        updateBusy();
    });

    // Left: settings, in a scroll area.
    auto* settings = new QWidget;
    auto* sl = new QVBoxLayout(settings);
    sl->setContentsMargins(0, 0, 6, 0);
    sl->addWidget(buildSourceGroup());
    sl->addWidget(buildCorrectionGroup());
    sl->addWidget(buildMosaicGroup());
    sl->addWidget(buildProcessingGroup());
    sl->addWidget(buildColourGroup());
    sl->addWidget(buildExportGroup());
    sl->addStretch(1);
    auto* scroll = new QScrollArea(this);
    scroll->setWidget(settings);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setMinimumWidth(settings->minimumSizeHint().width() + scroll->verticalScrollBar()->sizeHint().width() + 4);
    scroll->setMaximumWidth(520);

    // Centre: view bar, image, view options, read-out.
    auto* centre = new QWidget;
    auto* cl = new QVBoxLayout(centre);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->addWidget(buildViewBar());
    m_centre = new QStackedWidget(centre);
    m_dropHint = new QLabel(tr("Drop a raw 4 × 4 mosaic here (TIFF, PNG or .mat), or a cube (ENVI .hdr, .mat),\n"
                               "or use “Import from workspace” to take the image or calibrated image of a tab.\n\n"
                               "A spectral response file (.mat with lambda and QE_percent, or .csv) can be dropped too."),
                            m_centre);
    m_dropHint->setAlignment(Qt::AlignCenter);
    m_dropHint->setWordWrap(true);
    m_dropHint->setForegroundRole(QPalette::PlaceholderText);
    m_dropHint->setFrameShape(QFrame::StyledPanel);
    // Up to four views of the same cube, in step: zoom, pan, and a pointer
    // that mirrors the mouse.
    m_viewGrid = new QWidget(m_centre);
    m_gridLayout = new QGridLayout(m_viewGrid);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(4);
    updateGridStretch();
    for (int i = 0; i < kMaxPanes; ++i) {
        Pane& pane = m_panes[size_t(i)];
        pane.viewMode = kDefaultPaneModes[size_t(i)];
        auto* box = new QFrame(m_viewGrid);
        box->setObjectName(QStringLiteral("hsiPane%1").arg(i + 1));
        pane.box = box;
        auto* pl = new QVBoxLayout(pane.box);
        pl->setContentsMargins(2, 2, 2, 2);
        pl->setSpacing(2);
        pane.header = new QWidget(pane.box);
        pane.title = new QLabel(pane.header);
        pane.title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        pane.title->setToolTip(tr("Click a view to choose what it shows in View: and the options below the image"));
        auto* hl = new QHBoxLayout(pane.header);
        hl->setContentsMargins(4, 0, 0, 0);
        hl->setSpacing(2);
        hl->addWidget(pane.title, 1);
        pane.refresh = new QToolButton(pane.header);
        pane.refresh->setIcon(icons::icon(icons::Glyph::Refresh));
        pane.refresh->setAutoRaise(true);
        pane.refresh->setToolTip(tr("Refresh this view: draw it again from the current settings"));
        pane.reset = new QToolButton(pane.header);
        pane.reset->setIcon(icons::icon(icons::Glyph::Reset));
        pane.reset->setAutoRaise(true);
        pane.reset->setToolTip(tr("Reset this view: its default view mode, band and false-colour bands, fitted to the window"));
        hl->addWidget(pane.refresh);
        hl->addWidget(pane.reset);
        connect(pane.refresh, &QToolButton::clicked, this, [this, i] { refreshPane(i); });
        connect(pane.reset, &QToolButton::clicked, this, [this, i] { resetPane(i); });
        pl->addWidget(pane.header);
        pane.view = new ImageView(pane.box);
        pane.view->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        pane.view->setRegionTool(true);  // the spectrum tool is the default
        pl->addWidget(pane.view, 1);
        pane.header->hide();
        pane.box->setVisible(i == 0);
        // A click anywhere in a view makes it the active one.
        pane.header->installEventFilter(this);
        pane.view->viewport()->installEventFilter(this);
        connect(pane.view, &ImageView::hoveredPixel, this, [this, i](int x, int y) { onHover(i, x, y); });
        connect(pane.view, &ImageView::pixelClicked, this, [this, i](int x, int y) { onPixelClicked(i, x, y); });
        connect(pane.view, &ImageView::regionSelected, this, [this, i](const QRect& r) { onRegionSelected(i, r); });
        connect(pane.view, &ImageView::overlayRemoveRequested, this, [this, i](int index) { removeOverlay(i, index); });
        connect(pane.view, &ImageView::viewStateChanged, this, [this, i] { onPaneViewChanged(i); });
        connect(pane.view, &ImageView::imageSizeChanged, this, [this, i] { applySharedView(i); });
    }
    m_gridLayout->addWidget(m_panes[0].box, 0, 0);
    m_view = m_panes[0].view;
    // "Processing…" over the image while extraction, processing or
    // clustering runs (shown after a short delay, so quick changes don't flash it).
    m_busyOverlay = new QFrame(m_viewGrid);
    m_busyOverlay->setObjectName(QStringLiteral("hsiBusy"));
    m_busyOverlay->setStyleSheet(QStringLiteral("QFrame#hsiBusy { background: rgba(20, 20, 20, 190); border-radius: 8px; }"
                                                "QFrame#hsiBusy QLabel { color: white; }"));
    {
        auto* bl = new QHBoxLayout(m_busyOverlay);
        bl->setContentsMargins(14, 10, 14, 10);
        bl->setSpacing(10);
        auto* text = new QLabel(tr("Processing…"), m_busyOverlay);
        QFont f = text->font();
        f.setBold(true);
        text->setFont(f);
        auto* bar = new QProgressBar(m_busyOverlay);
        bar->setRange(0, 0);  // busy: moving bar, no percentage
        bar->setTextVisible(false);
        bar->setFixedSize(140, 12);
        bl->addWidget(text);
        bl->addWidget(bar);
    }
    m_busyOverlay->hide();
    m_viewGrid->installEventFilter(this);
    m_busyDelay = new QTimer(this);
    m_busyDelay->setSingleShot(true);
    m_busyDelay->setInterval(200);
    connect(m_busyDelay, &QTimer::timeout, this, [this] {
        if (isBusy())
            showBusyIndicator(true);
    });
    m_centre->addWidget(m_dropHint);
    m_centre->addWidget(m_viewGrid);
    cl->addWidget(m_centre, 1);
    cl->addWidget(buildViewOptions());
    {
        // As tall as its tallest page, so the views don't move when the page changes.
        int h = 0;
        for (int k = 0; k < m_viewOptions->count(); ++k)
            h = std::max(h, m_viewOptions->widget(k)->sizeHint().height());
        m_viewOptions->setFixedHeight(h);
    }
    m_readout = new QLabel(centre);
    m_readout->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_readout->setMinimumHeight(m_readout->fontMetrics().height() + 4);
    cl->addWidget(m_readout);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(scroll);
    splitter->addWidget(centre);
    m_rightTabs = new QTabWidget(splitter);
    m_rightTabs->setObjectName(QStringLiteral("hsiRightTabs"));
    m_rightTabs->setDocumentMode(true);
    m_rightTabs->addTab(buildSpectrumPanel(), tr("Spectra"));
    m_rightTabs->addTab(buildScorePanel(), tr("Scores (PCA)"));
    connect(m_rightTabs, &QTabWidget::currentChanged, this, [this] { updateScores(); });
    splitter->addWidget(m_rightTabs);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 1);
    splitter->setSizes({340, 900, 380});
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    // Saved choices: mosaic layout and extraction method.
    QSettings s;
    m_loadingSettings = true;
    if (const auto saved = MosaicLayout::fromString(s.value(QStringLiteral("hsi/layout")).toString())) {
        ExtractOptions o = extractOptions();
        o.layout = *saved;
        setExtractOptions(o);
    }
    m_method->setCurrentIndex(std::clamp(s.value(QStringLiteral("hsi/method"), 0).toInt(), 0, m_method->count() - 1));
    m_loadingSettings = false;
    updateBandControls();
    updateCubeInfo();
}

HsiWorkspace::~HsiWorkspace()
{
    // Let running work finish before the members go away.
    m_watcher->waitForFinished();
    m_clusterWatcher->waitForFinished();
}

QWidget* HsiWorkspace::buildSourceGroup()
{
    auto* g = new QGroupBox(tr("Source"));
    auto* l = new QVBoxLayout(g);
    auto* row = new QVBoxLayout;
    auto* open = new QPushButton(tr("Open file…"), g);
    open->setToolTip(tr("A raw mosaic (TIFF, PNG, JPEG, .mat), a cube (ENVI .hdr, .mat) or a spectral response file"));
    auto* import = new QPushButton(tr("Import from workspace…"), g);
    import->setToolTip(tr("Use the single image, light or dark frames, or calibrated image of an open tab"));
    row->addWidget(open);
    row->addWidget(import);
    l->addLayout(row);
    m_sourceLabel = hint(tr("No image yet. Drop a file onto the workspace."), g);
    l->addWidget(m_sourceLabel);
    connect(open, &QPushButton::clicked, this, &HsiWorkspace::openFileDialog);
    connect(import, &QPushButton::clicked, this, &HsiWorkspace::importFromWorkspace);
    return g;
}

QWidget* HsiWorkspace::buildCorrectionGroup()
{
    auto* g = new QGroupBox(tr("Correction"));
    auto* l = new QGridLayout(g);
    l->setColumnStretch(1, 1);
    int row = 0;
    auto* load = new QPushButton(tr("Load spectral response…"), g);
    load->setToolTip(tr("The reconstruction matrix: QE of every channel (.mat with lambda and QE_percent, or .csv "
                        "with Band1 … Band16 columns). It stays on your computer; it is not copied or exported."));
    auto* settings = new QToolButton(g);
    settings->setText(tr("Settings…"));
    settings->setToolTip(tr("Spectral response details, spectral correction, exposure times, sensor profile"));
    l->addWidget(load, row, 0, 1, 2);
    l->addWidget(settings, row++, 2);
    m_responseStatus = hint(tr("No spectral response."), g);
    l->addWidget(m_responseStatus, row++, 0, 1, 3);
    m_correctionCheck = new QCheckBox(tr("Spectral correction"), g);
    m_correctionCheck->setToolTip(tr("Turn the 16 overlapping channels into narrow bands at the channel peaks "
                                     "(details under Settings…). Needs the spectral response or a correction matrix."));
    m_correctionCheck->setEnabled(false);
    l->addWidget(m_correctionCheck, row++, 0, 1, 3);

    auto frameRow = [&](bool white) {
        auto* button = new QToolButton(g);
        button->setText(white ? tr("White frame") : tr("Dark frame"));
        button->setPopupMode(QToolButton::InstantPopup);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setToolTip(white ? tr("Optional: a capture of a white reference filling the view, for a per-pixel flat field "
                                      "(gives reflectance)")
                                 : tr("Optional: a capture with the lens covered, subtracted pixel by pixel (replaces the black level)"));
        auto* menu = new QMenu(button);
        menu->addAction(tr("From file…"), this, [this, white] {
            const QString path = QFileDialog::getOpenFileName(this, white ? tr("Open white reference frame") : tr("Open dark frame"),
                                                              lastDir(), tr("Images (*.tif *.tiff *.png *.mat);;All files (*)"));
            if (!path.isEmpty())
                loadReferenceFrame(white, path);
        });
        menu->addAction(tr("From workspace…"), this, [this, white] { importReferenceFrame(white); });
        button->setMenu(menu);
        auto* label = hint(tr("none"), g);
        auto* clear = new QToolButton(g);
        clear->setText(QStringLiteral("✕"));
        clear->setToolTip(white ? tr("Remove the white frame") : tr("Remove the dark frame"));
        clear->setEnabled(false);
        l->addWidget(button, row, 0);
        l->addWidget(label, row, 1);
        l->addWidget(clear, row++, 2);
        connect(clear, &QToolButton::clicked, this, [this, white] { clearReferenceFrame(white); });
        (white ? m_whiteLabel2 : m_darkLabel) = label;
        (white ? m_clearWhiteFrame : m_clearDark) = clear;
    };
    frameRow(false);
    frameRow(true);

    connect(load, &QPushButton::clicked, this, &HsiWorkspace::loadResponseDialog);
    connect(settings, &QToolButton::clicked, this, &HsiWorkspace::openCorrectionSettings);
    connect(m_correctionCheck, &QCheckBox::toggled, this, [this](bool on) {
        CorrectionSettings s = m_corr;
        s.spectralCorrection = on;
        applyCorrectionSettings(s);
    });
    return g;
}

QWidget* HsiWorkspace::buildMosaicGroup()
{
    m_mosaicGroup = new QGroupBox(tr("Mosaic → cube"));
    auto* g = m_mosaicGroup;
    auto* l = new QVBoxLayout(g);
    auto* layoutRow = new QHBoxLayout;
    m_layoutLabel = new QLabel(g);
    m_layoutLabel->setWordWrap(true);
    auto* layoutButton = new QPushButton(tr("Mosaic layout…"), g);
    layoutButton->setToolTip(tr("Which channel sits at each position of the 4 × 4 super-pixel"));
    layoutRow->addWidget(m_layoutLabel, 1);
    layoutRow->addWidget(layoutButton);
    l->addLayout(layoutRow);
    connect(layoutButton, &QPushButton::clicked, this, &HsiWorkspace::editMosaicLayout);

    auto* form = new QFormLayout;
    m_method = new QComboBox(g);
    m_method->addItem(tr("Sub-sample (as extract_hsi_channels.m)"));
    m_method->addItem(tr("Sub-sample + registration (recommended)"));
    m_method->addItem(tr("Full resolution: bilinear"));
    m_method->addItem(tr("Full resolution: PPID demosaic"));
    m_method->setItemData(0, tr("One value per super-pixel and channel: rows/4 × cols/4 × 16. The bands are up to ¾ "
                                "of a cube pixel apart, which gives false spectra at edges."), Qt::ToolTipRole);
    m_method->setItemData(1, tr("As sub-sampling, but every band is moved to the centre of the super-pixel, so all "
                                "bands describe the same point."), Qt::ToolTipRole);
    m_method->setItemData(2, tr("Every band interpolated to the full sensor resolution from its own pixels."),
                          Qt::ToolTipRole);
    m_method->setItemData(3, tr("Pseudo-panchromatic image difference (Mihoubi et al. 2017): a full-resolution image of "
                                "all channels guides the interpolation, so edges stay sharp and aligned."),
                          Qt::ToolTipRole);
    m_method->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_method->setMinimumContentsLength(18);
    form->addRow(tr("Method:"), m_method);
    m_blackLevel = doubleSpin(g, -100000, 100000, 0.0, 1, 1.0);
    m_blackLevel->setToolTip(tr("Dark offset in DN, subtracted from every pixel. Leave 0 for a calibrated image "
                                "(light − dark), which has it removed already."));
    form->addRow(tr("Black level (DN):"), m_blackLevel);
    m_bitDepth = intSpin(g, 0, 16, 14);
    m_bitDepth->setSpecialValueText(tr("no check"));
    m_bitDepth->setToolTip(tr("ADC bit depth. Values above 2^bits − 1 are taken as MSB-aligned in a 16-bit "
                              "container and shifted down, as hsi_reconstruct.m does."));
    form->addRow(tr("Bit depth:"), m_bitDepth);
    m_clipNegative = new QCheckBox(tr("Clip negative values to 0"), g);
    m_clipNegative->setChecked(true);
    form->addRow(QString(), m_clipNegative);
    l->addLayout(form);
    m_cubeInfo = hint(QString(), g);
    l->addWidget(m_cubeInfo);

    auto changed = [this] {
        if (m_loadingSettings)
            return;
        QSettings().setValue(QStringLiteral("hsi/method"), m_method->currentIndex());
        scheduleUpdate(true);
    };
    connect(m_method, &QComboBox::currentIndexChanged, this, changed);
    connect(m_blackLevel, &QDoubleSpinBox::valueChanged, this, changed);
    connect(m_bitDepth, &QSpinBox::valueChanged, this, changed);
    connect(m_clipNegative, &QCheckBox::toggled, this, changed);
    updateLayoutLabel();
    return g;
}

QWidget* HsiWorkspace::buildProcessingGroup()
{
    auto* g = new QGroupBox(tr("Processing"));
    auto* l = new QGridLayout(g);
    l->setColumnStretch(1, 1);
    int row = 0;

    m_reflectance = new QCheckBox(tr("Reflectance"), g);
    m_reflectance->setToolTip(tr("Divide every band by its mean over a white reference area: spectra become "
                                 "comparable between exposures and lights. Recommended first step."));
    auto* pickWhite = new QPushButton(tr("Pick white area"), g);
    pickWhite->setToolTip(tr("Then drag a rectangle over a white (or known grey) reference in the image"));
    m_clearWhite = new QPushButton(tr("Clear"), g);
    m_clearWhite->setToolTip(tr("Remove the white area and turn reflectance off"));
    m_clearWhite->setEnabled(false);
    l->addWidget(m_reflectance, row, 0);
    l->addWidget(pickWhite, row, 1);
    l->addWidget(m_clearWhite, row++, 2);
    m_whiteValue = doubleSpin(g, 0.01, 1.0, 1.0, 3, 0.01);
    m_whiteValue->setToolTip(tr("Reflectance of the reference (e.g. 0.99 for Spectralon, about 0.9 for white paper)"));
    auto* refLabel = new QLabel(tr("Reference reflectance:"), g);
    refLabel->setWordWrap(true);
    l->addWidget(refLabel, row, 0, 1, 2);
    l->addWidget(m_whiteValue, row++, 2);
    m_whiteLabel = hint(tr("No white area chosen."), g);
    l->addWidget(m_whiteLabel, row++, 0, 1, 3);

    m_mnf = new QCheckBox(tr("Spectral denoise (MNF)"), g);
    m_mnf->setToolTip(tr("Minimum noise fraction, as in ENVI: the bands are turned into components ordered by "
                         "signal-to-noise ratio, the noisiest are dropped, and the rest turned back into bands."));
    m_mnfKeep = intSpin(g, 1, kBands - 1, 6);
    m_mnfKeep->setToolTip(tr("Number of components to keep"));
    l->addWidget(m_mnf, row, 0, 1, 2);
    l->addWidget(m_mnfKeep, row++, 2);

    m_guided = new QCheckBox(tr("Edge-preserving denoise (guided)"), g);
    m_guided->setToolTip(tr("Guided filter with the mean of all bands as the guide: smooths noise while keeping the "
                            "edges of every band in the same place."));
    m_guidedRadius = intSpin(g, 1, 10, 2);
    m_guidedRadius->setPrefix(tr("r "));
    m_guidedEps = doubleSpin(g, 0.0001, 1.0, 0.01, 4, 0.005);
    m_guidedEps->setPrefix(QStringLiteral("ε "));
    l->addWidget(m_guided, row++, 0, 1, 3);
    l->addWidget(m_guidedRadius, row, 1);
    l->addWidget(m_guidedEps, row++, 2);

    m_antiAlias = new QCheckBox(tr("Anti-aliasing (low-pass)"), g);
    m_antiAlias->setToolTip(tr("Gaussian low-pass of every band: removes the coloured fringes that sampling every "
                               "band at 1/16 of the pixels causes. Registration or demosaicking helps more."));
    m_antiAliasSigma = doubleSpin(g, 0.2, 5.0, 0.7, 2, 0.1);
    m_antiAliasSigma->setPrefix(QStringLiteral("σ "));
    l->addWidget(m_antiAlias, row, 0, 1, 2);
    l->addWidget(m_antiAliasSigma, row++, 2);

    // Chemometric preprocessing of every pixel's spectrum (as in PLS_Toolbox,
    // Unscrambler, perClass): before PCA, classes and similarity.
    m_absorbance = new QCheckBox(tr("Absorbance (−log₁₀ R)"), g);
    m_absorbance->setToolTip(tr("Turn reflectance into absorbance, which is roughly proportional to concentration "
                                "(Beer–Lambert). Use with reflectance or a white frame."));
    l->addWidget(m_absorbance, row++, 0, 1, 3);
    m_normalisation = new QComboBox(g);
    m_normalisation->addItem(tr("None"));
    m_normalisation->addItem(tr("SNV"));
    m_normalisation->addItem(tr("MSC"));
    m_normalisation->addItem(tr("Unit length"));
    m_normalisation->addItem(tr("Unit area"));
    m_normalisation->addItem(tr("Maximum = 1"));
    m_normalisation->setItemData(1, tr("Standard normal variate: every spectrum minus its mean, divided by its standard "
                                       "deviation. Removes brightness and scatter differences (shading, surface)."), Qt::ToolTipRole);
    m_normalisation->setItemData(2, tr("Multiplicative scatter correction: every spectrum fitted to the mean spectrum "
                                       "(offset + slope) and corrected by the fit."), Qt::ToolTipRole);
    m_normalisation->setItemData(3, tr("Every spectrum divided by its length: keeps only the shape (as the spectral angle)."), Qt::ToolTipRole);
    m_normalisation->setItemData(4, tr("Every spectrum divided by the sum of its values."), Qt::ToolTipRole);
    m_normalisation->setItemData(5, tr("Every spectrum divided by its largest value."), Qt::ToolTipRole);
    m_normalisation->setToolTip(tr("Spectrum normalisation: removes differences in brightness, so spectra compare by shape"));
    l->addWidget(new QLabel(tr("Normalise:"), g), row, 0);
    l->addWidget(m_normalisation, row++, 1, 1, 2);
    m_derivative = new QComboBox(g);
    m_derivative->addItems({tr("No smoothing"), tr("Smooth"), tr("1st derivative"), tr("2nd derivative")});
    m_derivative->setToolTip(tr("Savitzky–Golay filter along the spectrum: smoothing, or a derivative, which removes "
                                "offsets (1st) and slopes (2nd) and sharpens overlapping features. With 16 bands, keep "
                                "the window small."));
    m_sgWindow = intSpin(g, 3, 15, 5);
    m_sgWindow->setSingleStep(2);
    m_sgWindow->setPrefix(tr("window "));
    m_sgWindow->setToolTip(tr("Number of bands in the window (odd)"));
    m_sgPolynomial = intSpin(g, 1, 5, 2);
    m_sgPolynomial->setPrefix(tr("order "));
    m_sgPolynomial->setToolTip(tr("Order of the polynomial fitted in the window"));
    l->addWidget(new QLabel(tr("Savitzky–Golay:"), g), row, 0);
    l->addWidget(m_derivative, row++, 1, 1, 2);
    l->addWidget(m_sgWindow, row, 1);
    l->addWidget(m_sgPolynomial, row++, 2);

    m_upscale = new QComboBox(g);
    for (int f : {1, 2, 3, 4})
        m_upscale->addItem(QStringLiteral("%1×").arg(f), f);
    m_upscale->setToolTip(tr("Enlarge every band. Interpolation only: no new detail. For more resolution, choose a "
                             "full-resolution extraction method instead."));
    m_interpolation = new QComboBox(g);
    m_interpolation->addItems({tr("Nearest"), tr("Bilinear"), tr("Bicubic")});
    m_interpolation->setCurrentIndex(2);
    l->addWidget(new QLabel(tr("Upscale:"), g), row, 0);
    l->addWidget(m_upscale, row, 1);
    l->addWidget(m_interpolation, row++, 2);
    l->addWidget(hint(tr("Order: reflectance → MNF → guided → anti-aliasing → absorbance → normalise → "
                         "Savitzky–Golay → upscale."), g), row++, 0, 1, 3);

    auto changed = [this] { scheduleUpdate(false); };
    for (QCheckBox* c : {m_reflectance, m_mnf, m_guided, m_antiAlias, m_absorbance})
        connect(c, &QCheckBox::toggled, this, changed);
    for (QSpinBox* s : {m_mnfKeep, m_guidedRadius, m_sgPolynomial})
        connect(s, &QSpinBox::valueChanged, this, changed);
    connect(m_sgWindow, &QSpinBox::valueChanged, this, [this, changed](int v) {
        if (v % 2 == 0) {  // odd only
            m_sgWindow->setValue(v + 1);
            return;
        }
        changed();
    });
    connect(m_normalisation, &QComboBox::currentIndexChanged, this, changed);
    connect(m_derivative, &QComboBox::currentIndexChanged, this, [this, changed](int i) {
        m_sgWindow->setEnabled(i > 0);
        m_sgPolynomial->setEnabled(i > 0);
        changed();
    });
    m_sgWindow->setEnabled(false);
    m_sgPolynomial->setEnabled(false);
    for (QDoubleSpinBox* s : {m_whiteValue, m_guidedEps, m_antiAliasSigma})
        connect(s, &QDoubleSpinBox::valueChanged, this, changed);
    connect(m_upscale, &QComboBox::currentIndexChanged, this, changed);
    connect(m_interpolation, &QComboBox::currentIndexChanged, this, changed);
    connect(pickWhite, &QPushButton::clicked, this, [this] { setTool(Tool::WhiteReference); });
    connect(m_clearWhite, &QPushButton::clicked, this, &HsiWorkspace::clearWhiteReference);
    // The white area is drawn only while reflectance is on.
    connect(m_reflectance, &QCheckBox::toggled, this, &HsiWorkspace::updateOverlays);
    return g;
}

QWidget* HsiWorkspace::buildColourGroup()
{
    auto* g = new QGroupBox(tr("Colour reconstruction"));
    auto* l = new QVBoxLayout(g);
    l->addWidget(hint(tr("Needs the spectral response (Correction panel)."), g));
    auto* form = new QFormLayout;
    m_sigma = doubleSpin(g, 2.0, 100.0, 20.0, 1, 1.0);
    m_sigma->setSuffix(QStringLiteral(" nm"));
    m_sigma->setToolTip(tr("Correlation length of the smoothness prior of the spectral estimate"));
    form->addRow(tr("Smoothness σ:"), m_sigma);
    m_alpha = doubleSpin(g, 0.0001, 10.0, 0.05, 4, 0.01);
    m_alpha->setToolTip(tr("Regularisation, relative to the mean channel energy: larger is smoother and less noisy"));
    form->addRow(tr("Regularisation α:"), m_alpha);
    m_wb = new QComboBox(g);
    m_wb->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_wb->setMinimumContentsLength(12);
    m_wb->addItems({tr("Grey world"), tr("White patch"), tr("Reference area"), tr("None (equal-energy light)")});
    form->addRow(tr("White balance:"), m_wb);
    m_pickColourRef = new QPushButton(tr("Pick reference area"), g);
    form->addRow(QString(), m_pickColourRef);
    m_exposure = doubleSpin(g, 50.0, 100.0, 99.0, 1, 0.5);
    m_exposure->setSuffix(QStringLiteral(" %"));
    m_exposure->setToolTip(tr("This percentile of max(R, G, B) is shown as full brightness"));
    form->addRow(tr("Exposure:"), m_exposure);
    l->addLayout(form);
    auto changed = [this] {
        if (sender() == m_sigma || sender() == m_alpha)
            m_xyz.clear();
        invalidateColour();
        if (viewMode() == ViewMode::Colour)
            renderView();
    };
    connect(m_sigma, &QDoubleSpinBox::valueChanged, this, changed);
    connect(m_alpha, &QDoubleSpinBox::valueChanged, this, changed);
    connect(m_wb, &QComboBox::currentIndexChanged, this, changed);
    connect(m_exposure, &QDoubleSpinBox::valueChanged, this, changed);
    connect(m_pickColourRef, &QPushButton::clicked, this, [this] { setTool(Tool::ColourReference); });
    return g;
}

QWidget* HsiWorkspace::buildExportGroup()
{
    auto* g = new QGroupBox(tr("Export"));
    auto* l = new QGridLayout(g);
    m_exportFormat = new QComboBox(g);
    m_exportFormat->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_exportFormat->setMinimumContentsLength(10);
    m_exportFormat->addItem(tr("ENVI (.hdr + .img)"));
    m_exportFormat->addItem(tr("MATLAB (.mat)"));
    m_exportFormat->addItem(tr("Multi-page TIFF"));
    m_exportFormat->setItemData(0, tr("The standard exchange format for hyperspectral cubes: ENVI, Spectral Python, "
                                      "MATLAB hypercube, QGIS / GDAL"), Qt::ToolTipRole);
    m_exportType = new QComboBox(g);
    m_exportType->addItem(tr("32-bit float"));
    m_exportType->addItem(tr("16-bit unsigned"));
    m_exportType->setItemData(1, tr("Rounded and clipped to 0 … 65535: for raw counts only"), Qt::ToolTipRole);
    m_exportUnprocessed = new QCheckBox(tr("Unprocessed cube"), g);
    m_exportUnprocessed->setToolTip(tr("Save the cube as extracted, without the processing steps"));
    auto* cubeButton = new QPushButton(tr("Save cube…"), g);
    auto* viewButton = new QPushButton(tr("Save view (.png)…"), g);
    auto* spectraButton = new QPushButton(tr("Save spectra (.csv)…"), g);
    l->addWidget(m_exportFormat, 0, 0);
    l->addWidget(m_exportType, 0, 1);
    l->addWidget(m_exportUnprocessed, 1, 0);
    l->addWidget(cubeButton, 1, 1);
    l->addWidget(viewButton, 2, 0);
    l->addWidget(spectraButton, 2, 1);
    connect(cubeButton, &QPushButton::clicked, this, &HsiWorkspace::exportCubeDialog);
    connect(viewButton, &QPushButton::clicked, this, &HsiWorkspace::exportViewDialog);
    connect(spectraButton, &QPushButton::clicked, this, &HsiWorkspace::exportSpectraDialog);
    return g;
}

QWidget* HsiWorkspace::buildViewBar()
{
    auto* bar = new QWidget;
    auto* l = new QHBoxLayout(bar);
    l->setContentsMargins(0, 0, 0, 2);
    m_viewMode = new QComboBox(bar);
    m_viewMode->setObjectName(QStringLiteral("hsiViewMode"));
    fillViewModes(m_viewMode);
    m_viewMode->setCurrentIndex(int(ViewMode::Band));
    l->addWidget(new QLabel(tr("View:"), bar));
    l->addWidget(m_viewMode);
    l->addSpacing(12);
    m_toolCombo = new QComboBox(bar);
    m_toolCombo->addItem(tr("Pan and zoom"));
    m_toolCombo->addItem(tr("Spectrum: click a pixel or drag an area"));
    m_toolCombo->addItem(tr("Pick white area"));
    m_toolCombo->addItem(tr("Pick colour reference area"));
    m_toolCombo->addItem(tr("Pick reference for similar areas"));
    m_toolCombo->setItemData(int(Tool::Spectrum), tr("Click a pixel or drag an area to keep its spectrum. The wheel zooms; "
                                                     "drag with the middle button or with Ctrl held to pan. Right-click a "
                                                     "box's label to remove it."), Qt::ToolTipRole);
    m_toolCombo->setCurrentIndex(int(Tool::Spectrum));
    m_toolCombo->setToolTip(m_toolCombo->itemData(int(Tool::Spectrum), Qt::ToolTipRole).toString());
    l->addWidget(new QLabel(tr("Mouse:"), bar));
    l->addWidget(m_toolCombo);
    l->addSpacing(12);
    m_paneCountCombo = new QComboBox(bar);
    m_paneCountCombo->setObjectName(QStringLiteral("hsiViews"));
    m_paneCountCombo->addItem(tr("1 view"), 1);
    m_paneCountCombo->addItem(tr("2 views"), 2);
    m_paneCountCombo->addItem(tr("4 views"), 4);
    m_paneCountCombo->setToolTip(tr("Show the cube in two or four views side by side. Click a view to make it the "
                                    "active one: View: and the options below the image then set what it shows. "
                                    "Zoom, pan and the mouse pointer follow in all of them."));
    l->addWidget(m_paneCountCombo);
    l->addStretch(1);
    auto* refreshAllButton = new QToolButton(bar);
    refreshAllButton->setObjectName(QStringLiteral("hsiRefreshAll"));
    refreshAllButton->setIcon(icons::icon(icons::Glyph::Refresh));
    refreshAllButton->setText(tr("Refresh"));
    refreshAllButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    refreshAllButton->setToolTip(tr("Make the cube again from the source with the current settings (extraction, "
                                    "correction, processing) and draw every view again"));
    auto* resetAllButton = new QToolButton(bar);
    resetAllButton->setObjectName(QStringLiteral("hsiResetAll"));
    resetAllButton->setIcon(icons::icon(icons::Glyph::Reset));
    resetAllButton->setText(tr("Reset"));
    resetAllButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    resetAllButton->setToolTip(tr("Turn all processing off, set colour reconstruction back to its defaults and every "
                                  "view back to its default mode, band and zoom. The source, extraction settings, "
                                  "spectral response, correction, reference frames and drawn areas are kept."));
    l->addWidget(refreshAllButton);
    l->addWidget(resetAllButton);
    connect(refreshAllButton, &QToolButton::clicked, this, &HsiWorkspace::refreshAll);
    connect(resetAllButton, &QToolButton::clicked, this, [this] {
        if (QMessageBox::question(this, tr("Reset"),
                                  tr("Turn all processing off and set colour reconstruction and every view back to "
                                     "their defaults?"))
            == QMessageBox::Yes)
            resetAll();
    });
    m_abButton = new QToolButton(bar);
    m_abButton->setText(QStringLiteral("A/B"));
    m_abButton->setCheckable(true);
    m_abButton->setToolTip(tr("Compare with the unprocessed cube: drag the divider across the image"));
    l->addWidget(m_abButton);
    connect(m_viewMode, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_viewOptions->setCurrentIndex(i);
        m_panes[size_t(m_activePane)].viewMode = ViewMode(i);
        updatePaneTitles();
        renderView();
        updateSpectra();
    });
    connect(m_paneCountCombo, &QComboBox::currentIndexChanged, this,
            [this] { setPaneCount(m_paneCountCombo->currentData().toInt()); });
    connect(m_toolCombo, &QComboBox::currentIndexChanged, this, [this](int i) { setTool(Tool(i)); });
    connect(m_abButton, &QToolButton::toggled, this, [this] { renderView(); });
    return bar;
}

QWidget* HsiWorkspace::buildViewOptions()
{
    m_viewOptions = new QStackedWidget;
    auto page = [this](QLayout* layout) {
        auto* w = new QWidget(m_viewOptions);
        layout->setContentsMargins(0, 2, 0, 2);
        w->setLayout(layout);
        m_viewOptions->addWidget(w);
        return w;
    };
    // Mosaic
    {
        auto* l = new QHBoxLayout;
        l->addWidget(hint(tr("The raw sensor image, before extraction."), nullptr));
        page(l);
    }
    // Band
    {
        auto* l = new QHBoxLayout;
        m_bandSlider = new QSlider(Qt::Horizontal);
        m_bandSlider->setRange(0, kBands - 1);
        m_bandLabel = new QLabel;
        m_bandLabel->setMinimumWidth(160);
        l->addWidget(new QLabel(tr("Band:")));
        l->addWidget(m_bandSlider, 1);
        l->addWidget(m_bandLabel);
        page(l);
        connect(m_bandSlider, &QSlider::valueChanged, this, [this](int v) {
            m_panes[size_t(m_activePane)].band = v;
            updateBandControls();
            updatePaneTitles();
            renderView();
        });
    }
    // Gallery
    {
        auto* l = new QHBoxLayout;
        l->addWidget(hint(tr("All bands with one shared contrast, channel 1 at the top left."), nullptr));
        page(l);
    }
    // Colour
    {
        auto* l = new QHBoxLayout;
        l->addWidget(hint(tr("Spectrum per pixel from the spectral response (Wiener estimate) → CIE 1931 XYZ → white "
                             "balance → sRGB, as hsi_reconstruct.m. Without a spectral response, a natural false-colour picture is shown."), nullptr));
        page(l);
    }
    // False colour
    {
        auto* l = new QHBoxLayout;
        const char* names[] = {"R", "G", "B"};
        for (int i = 0; i < 3; ++i) {
            m_rgbBands[size_t(i)] = new QComboBox;
            l->addWidget(new QLabel(QLatin1String(names[i])));
            l->addWidget(m_rgbBands[size_t(i)]);
            connect(m_rgbBands[size_t(i)], &QComboBox::currentIndexChanged, this, [this] {
                Pane& active = m_panes[size_t(m_activePane)];
                for (int c = 0; c < 3; ++c)
                    active.rgb[size_t(c)] = m_rgbBands[size_t(c)]->currentIndex();
                if (viewMode() == ViewMode::FalseColour)
                    renderView();
            });
        }
        auto* natural = new QPushButton(tr("Natural"));
        natural->setToolTip(tr("About 610, 550 and 490 nm"));
        auto* contrast = new QPushButton(tr("Most contrast"));
        contrast->setToolTip(tr("The three least correlated bands"));
        l->addWidget(natural);
        l->addWidget(contrast);
        l->addStretch(1);
        page(l);
        connect(natural, &QPushButton::clicked, this, [this] {
            int r, g, b;
            pseudoTrueColourBands(m_cube, r, g, b);
            setFalseColourBands(r, g, b);
        });
        connect(contrast, &QPushButton::clicked, this, [this] {
            int r, g, b;
            maxContrastBands(m_cube, r, g, b);
            setFalseColourBands(r, g, b);
        });
    }
    // PCA
    {
        auto* l = new QHBoxLayout;
        m_pcaLabel = hint(QString(), nullptr);
        l->addWidget(m_pcaLabel, 1);
        page(l);
    }
    // Index
    {
        auto* l = new QHBoxLayout;
        m_indexPreset = new QComboBox;
        m_indexPreset->addItem(tr("Custom"));
        for (const IndexPreset& p : indexPresets()) {
            m_indexPreset->addItem(p.name, p.formula);
            m_indexPreset->setItemData(m_indexPreset->count() - 1,
                                       QStringLiteral("%1\n%2").arg(p.formula, p.description), Qt::ToolTipRole);
        }
        m_indexFormula = new QLineEdit;
        m_indexFormula->setPlaceholderText(QStringLiteral("(b7 - b11) / (b7 + b11)"));
        m_indexFormula->setToolTip(tr("Bands as b1 … b16, or R<nm> for the band nearest a wavelength (needs the "
                                      "spectral response); + − × / ^ ( ), sqrt, abs, log, exp"));
        m_colormap = new QComboBox;
        m_colormap->addItems({tr("Grey"), QStringLiteral("Viridis"), QStringLiteral("Turbo"), tr("Diverging")});
        m_colormap->setCurrentIndex(1);
        m_indexLabel = new QLabel;
        l->addWidget(m_indexPreset);
        l->addWidget(m_indexFormula, 1);
        l->addWidget(m_colormap);
        auto* v = new QVBoxLayout;
        v->addLayout(l);
        v->addWidget(m_indexLabel);
        page(v);
        connect(m_indexPreset, &QComboBox::currentIndexChanged, this, [this](int i) {
            if (i > 0)
                setIndexFormula(m_indexPreset->itemData(i).toString());
        });
        connect(m_indexFormula, &QLineEdit::editingFinished, this, [this] { setIndexFormula(m_indexFormula->text()); });
        connect(m_colormap, &QComboBox::currentIndexChanged, this, [this] {
            if (viewMode() == ViewMode::Index)
                renderView();
        });
    }
    // Similarity
    {
        auto* l = new QHBoxLayout;
        m_simThreshold = doubleSpin(nullptr, 0.1, 45.0, 5.0, 1, 0.5);
        m_simThreshold->setSuffix(QStringLiteral("°"));
        m_simThreshold->setToolTip(tr("Pixels whose spectrum is within this angle of the reference are marked"));
        m_simDisplay = new QComboBox;
        m_simDisplay->addItems({tr("Marked on the image"), tr("Angle map")});
        auto* pick = new QPushButton(tr("Pick reference"));
        m_simLabel = new QLabel(tr("Choose a reference: click a pixel or drag an area."));
        l->addWidget(pick);
        l->addWidget(new QLabel(tr("Threshold:")));
        l->addWidget(m_simThreshold);
        l->addWidget(m_simDisplay);
        l->addWidget(m_simLabel, 1);
        page(l);
        connect(pick, &QPushButton::clicked, this, [this] { setTool(Tool::SimilarityReference); });
        connect(m_simThreshold, &QDoubleSpinBox::valueChanged, this, [this] {
            renderView();
            updateSpectra();
        });
        connect(m_simDisplay, &QComboBox::currentIndexChanged, this, [this] { renderView(); });
    }
    // Classes
    {
        auto* l = new QHBoxLayout;
        m_clusterK = intSpin(nullptr, 2, 12, 4);
        m_clusterK->setPrefix(tr("k = "));
        m_clusterShape = new QCheckBox(tr("By spectral shape (ignore brightness)"));
        m_clusterShape->setChecked(true);
        auto* run = new QPushButton(tr("Find classes"));
        m_clusterLabel = new QLabel;
        m_clusterLabel->setWordWrap(true);
        l->addWidget(m_clusterK);
        l->addWidget(m_clusterShape);
        l->addWidget(run);
        l->addWidget(m_clusterLabel, 1);
        page(l);
        connect(run, &QPushButton::clicked, this, [this] { runClustering(m_clusterK->value()); });
    }
    // Match to drawn areas
    {
        auto* l = new QHBoxLayout;
        m_matchAngle = doubleSpin(nullptr, 0.5, 45.0, 8.0, 1, 0.5);
        m_matchAngle->setSuffix(QStringLiteral("°"));
        m_matchAngle->setToolTip(tr("Pixels further than this angle from every drawn area are left unmatched"));
        m_matchShade = new QCheckBox(tr("Shade by similarity"));
        m_matchShade->setChecked(true);
        m_matchShade->setToolTip(tr("Full colour where the spectrum is the same as the area's, darker the more it differs"));
        m_matchBackground = new QComboBox;
        m_matchBackground->addItems({tr("Unmatched: image"), tr("Unmatched: black")});
        m_matchLabel = new QLabel;
        m_matchLabel->setWordWrap(true);
        l->addWidget(new QLabel(tr("Max. angle:")));
        l->addWidget(m_matchAngle);
        l->addWidget(m_matchShade);
        l->addWidget(m_matchBackground);
        l->addWidget(m_matchLabel, 1);
        page(l);
        auto redraw = [this] {
            if (anyPaneShows(ViewMode::Matches))
                renderView();
        };
        connect(m_matchAngle, &QDoubleSpinBox::valueChanged, this, redraw);
        connect(m_matchShade, &QCheckBox::toggled, this, redraw);
        connect(m_matchBackground, &QComboBox::currentIndexChanged, this, redraw);
    }
    m_viewOptions->setCurrentIndex(int(ViewMode::Band));
    return m_viewOptions;
}

QWidget* HsiWorkspace::buildSpectrumPanel()
{
    auto* w = new QWidget;
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(6, 0, 0, 0);
    m_plot = new SpectrumPlot(w);
    l->addWidget(m_plot, 3);
    auto* opts = new QHBoxLayout;
    m_plotWavelength = new QCheckBox(tr("Wavelength"), w);
    m_plotWavelength->setChecked(true);
    m_plotWavelength->setToolTip(tr("x axis in nm (needs the spectral response), otherwise band numbers"));
    m_plotNormalized = new QCheckBox(tr("Normalise"), w);
    m_plotNormalized->setToolTip(tr("Divide each curve by its maximum, to compare shapes"));
    m_plotClasses = new QCheckBox(tr("Class means"), w);
    m_plotClasses->setChecked(true);
    opts->addWidget(m_plotWavelength);
    opts->addWidget(m_plotNormalized);
    opts->addWidget(m_plotClasses);
    opts->addStretch(1);
    l->addLayout(opts);
    l->addWidget(new QLabel(tr("Kept points and areas:"), w));
    m_roiList = new QListWidget(w);
    l->addWidget(m_roiList, 1);
    auto* buttons = new QHBoxLayout;
    auto* remove = new QPushButton(tr("Remove"), w);
    auto* clear = new QPushButton(tr("Remove all"), w);
    buttons->addWidget(remove);
    buttons->addWidget(clear);
    buttons->addStretch(1);
    l->addLayout(buttons);
    connect(m_plotWavelength, &QCheckBox::toggled, m_plot, &SpectrumPlot::setShowWavelength);
    connect(m_plotNormalized, &QCheckBox::toggled, m_plot, &SpectrumPlot::setNormalized);
    connect(m_plotClasses, &QCheckBox::toggled, this, &HsiWorkspace::updateSpectra);
    connect(remove, &QPushButton::clicked, this, [this] {
        const int row = m_roiList->currentRow();
        if (row >= 0 && row < m_rois.size())
            removeRoi(row);
        else if (row >= m_rois.size())
            removeKeptSelection(row - int(m_rois.size()));
    });
    connect(clear, &QPushButton::clicked, this, &HsiWorkspace::clearRois);
    return w;
}

QWidget* HsiWorkspace::buildScorePanel()
{
    auto* w = new QWidget;
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(6, 4, 0, 0);
    auto* axes = new QHBoxLayout;
    m_scoreX = new QComboBox(w);
    m_scoreY = new QComboBox(w);
    m_scoreX->setObjectName(QStringLiteral("hsiScoreX"));
    m_scoreY->setObjectName(QStringLiteral("hsiScoreY"));
    axes->addWidget(new QLabel(tr("x:"), w));
    axes->addWidget(m_scoreX, 1);
    axes->addWidget(new QLabel(tr("y:"), w));
    axes->addWidget(m_scoreY, 1);
    l->addLayout(axes);
    m_scorePlot = new ScorePlot(w);
    m_scorePlot->setPlaceholder(tr("Load an image to see its principal component scores."));
    m_scorePlot->setToolTip(tr("Every pixel as a point (brighter: more pixels). Drag a loop around a group of points to "
                               "select those pixels in the image; right-click or Esc removes the selection. The kept "
                               "areas show in their colours."));
    l->addWidget(m_scorePlot, 3);
    auto* row = new QHBoxLayout;
    m_scoreInfo = hint(tr("No selection."), w);
    m_clearSelection = new QPushButton(tr("Clear selection"), w);
    m_clearSelection->setEnabled(false);
    m_keepSelection = new QPushButton(tr("Keep"), w);
    m_keepSelection->setToolTip(tr("Keep the mean spectrum of the selection in the list of spectra"));
    m_keepSelection->setEnabled(false);
    row->addWidget(m_scoreInfo, 1);
    row->addWidget(m_keepSelection);
    row->addWidget(m_clearSelection);
    l->addLayout(row);
    l->addWidget(new QLabel(tr("Loadings (what each component weighs):"), w));
    m_loadingsPlot = new SpectrumPlot(w);
    m_loadingsPlot->setYLabel(tr("Loading"));
    m_loadingsPlot->setMinimumHeight(120);
    l->addWidget(m_loadingsPlot, 2);
    auto axesChanged = [this] {
        m_scoreXIndex = m_scoreYIndex = -1;  // recompute
        updateScores();
    };
    connect(m_scoreX, &QComboBox::currentIndexChanged, this, axesChanged);
    connect(m_scoreY, &QComboBox::currentIndexChanged, this, axesChanged);
    connect(m_scorePlot, &ScorePlot::selectionDrawn, this, &HsiWorkspace::selectScores);
    connect(m_scorePlot, &ScorePlot::selectionCleared, this, &HsiWorkspace::clearScoreSelection);
    connect(m_clearSelection, &QPushButton::clicked, this, &HsiWorkspace::clearScoreSelection);
    connect(m_keepSelection, &QPushButton::clicked, this, &HsiWorkspace::keepScoreSelection);
    return w;
}

// ---------------------------------------------------------------------------
// Sources
// ---------------------------------------------------------------------------

void HsiWorkspace::report(const QString& message)
{
    emit statusMessage(message);
}

QString HsiWorkspace::lastDir() const
{
    return QSettings().value(QStringLiteral("hsi/lastDir"), QDir::homePath()).toString();
}

void HsiWorkspace::rememberDir(const QString& path)
{
    QSettings().setValue(QStringLiteral("hsi/lastDir"), QFileInfo(path).absolutePath());
}

QString HsiWorkspace::baseName() const
{
    return m_sourceName.isEmpty() ? QStringLiteral("hsi") : m_sourceName;
}

void HsiWorkspace::openFileDialog()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open a raw mosaic, a cube or a spectral response"), lastDir(),
        tr("Hyperspectral files (*.tif *.tiff *.png *.jpg *.jpeg *.mat *.hdr *.img *.csv *.txt);;All files (*)"));
    if (!path.isEmpty())
        loadFile(path);
}

bool HsiWorkspace::loadFile(const QString& path)
{
    rememberDir(path);
    const QString name = QFileInfo(path).fileName();
    const QString suffix = QFileInfo(path).suffix().toLower();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    auto restore = qScopeGuard([] { QApplication::restoreOverrideCursor(); });

    if (isResponseCandidate(path))
        return loadResponse(path);
    if (isCubeFile(path)) {
        const CubeFile f = readCube(path);
        if (!f.ok()) {
            report(f.error);
            return false;
        }
        m_sourceName = QFileInfo(path).completeBaseName();
        setCube(f.cube, tr("Cube from %1 (%2 × %3 × %4)").arg(name).arg(f.cube.rows()).arg(f.cube.cols()).arg(f.cube.bands()));
        m_cubeLayout = f.layout;
        return true;
    }
    if (suffix == QLatin1String("mat")) {
        const MatFileContents c = MatFileReader::read(path);
        if (!c.ok()) {
            report(c.error);
            return false;
        }
        bool hasResponse = false, hasCube = false;
        for (const MatVariable& v : c.variables) {
            if (v.name == QLatin1String("QE_percent") || v.name == QLatin1String("reconRGB")
                || (v.name == QLatin1String("QE") && v.data.frameCount() == 1))
                hasResponse = true;
            if (v.supported && v.isImageStack() && (v.name == QLatin1String("cube") || v.data.frameCount() == kBands))
                hasCube = true;
        }
        if (hasCube) {
            const CubeFile f = readMat(path);
            if (!f.ok()) {
                report(f.error);
                return false;
            }
            m_sourceName = QFileInfo(path).completeBaseName();
            setCube(f.cube, tr("Cube from %1 (%2 × %3 × %4)").arg(name).arg(f.cube.rows()).arg(f.cube.cols()).arg(f.cube.bands()));
            m_cubeLayout = f.layout;
            return true;
        }
        if (hasResponse)
            return loadResponse(path);
        const QList<MatVariable> stacks = c.imageStacks();
        if (stacks.isEmpty()) {
            report(tr("%1 holds no image.").arg(name));
            return false;
        }
        const MatVariable& v = stacks.first();
        m_sourceName = QFileInfo(path).completeBaseName();
        QString desc = tr("%1 · %2 · %3").arg(name, v.name, sizeText(v.data.rows(), v.data.cols()));
        if (v.data.frameCount() > 1)
            desc += tr(" · frame 1 of %1 (use a tab and “Import from workspace” for the mean)").arg(v.data.frameCount());
        setMosaic(v.data.frame(0), desc);
        return true;
    }
    const ImageFrame f = ImageFileReader::read(path);
    if (!f.ok()) {
        report(f.error);
        return false;
    }
    m_sourceName = QFileInfo(path).completeBaseName();
    QString desc = tr("%1 · %2 %3").arg(name, sizeText(f.data.rows(), f.data.cols()), f.matlabClass);
    if (!f.note.isEmpty())
        desc += QStringLiteral(" · ") + f.note;
    setMosaic(f.data.frame(0), desc);
    return true;
}

void HsiWorkspace::importFromWorkspace()
{
    const QList<ImportCandidate> candidates = m_importProvider ? m_importProvider() : QList<ImportCandidate>();
    ImportImageDialog dialog(candidates, this);
    if (dialog.exec() != QDialog::Accepted || dialog.selectedIndex() < 0)
        return;
    importCandidate(candidates[dialog.selectedIndex()]);
}

bool HsiWorkspace::importCandidate(const ImportCandidate& c)
{
    if (c.image.isNull())
        return false;
    QString tab = c.tabName;
    tab.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]+")), QStringLiteral("_"));
    m_sourceName = tab;
    setMosaic(c.image, tr("%1 · %2 · %3").arg(c.tabName, c.imageName, c.info));
    return true;
}

void HsiWorkspace::setMosaic(const Image2D& raw, const QString& description)
{
    m_mosaic = raw;
    m_fromMosaic = true;
    m_sourceInfo = description;
    m_sourceLabel->setText(description);
    m_mosaicGroup->setEnabled(true);
    m_viewMode->setItemData(int(ViewMode::Mosaic), QVariant(), Qt::UserRole - 1);  // enabled
    m_centre->setCurrentWidget(m_viewGrid);
    scheduleUpdate(true);
}

void HsiWorkspace::setCube(const Cube& cube, const QString& description)
{
    m_watcher->waitForFinished();
    m_mosaic = Image2D();
    m_fromMosaic = false;
    m_cubeLayout.reset();
    m_sourceInfo = description;
    m_sourceLabel->setText(description);
    m_cubeExtracted = cube;
    m_cubeRaw = Cube();
    m_needCorrect = true;
    m_mosaicGroup->setEnabled(false);
    m_viewMode->setItemData(int(ViewMode::Mosaic), 0, Qt::UserRole - 1);  // disabled: no mosaic
    if (viewMode() == ViewMode::Mosaic)
        m_viewMode->setCurrentIndex(int(ViewMode::Band));
    m_centre->setCurrentWidget(m_viewGrid);
    scheduleUpdate(false);
}

// ---------------------------------------------------------------------------
// Extraction and processing
// ---------------------------------------------------------------------------

ExtractOptions HsiWorkspace::extractOptions() const
{
    ExtractOptions o;
    o.layout = m_layout;
    o.blackLevel = m_blackLevel->value();
    o.bitDepth = m_bitDepth->value();
    o.clipNegative = m_clipNegative->isChecked();
    o.method = ExtractMethod(m_method->currentIndex());
    return o;
}

void HsiWorkspace::setExtractOptions(const ExtractOptions& o)
{
    const bool wasLoading = m_loadingSettings;
    m_loadingSettings = true;
    m_layout = o.layout;
    updateLayoutLabel();
    m_blackLevel->setValue(o.blackLevel);
    m_bitDepth->setValue(o.bitDepth);
    m_clipNegative->setChecked(o.clipNegative);
    m_method->setCurrentIndex(int(o.method));
    m_loadingSettings = wasLoading;
    if (!m_loadingSettings) {
        if (m_layout.isValid())
            QSettings().setValue(QStringLiteral("hsi/layout"), m_layout.toString());
        QSettings().setValue(QStringLiteral("hsi/method"), m_method->currentIndex());
        scheduleUpdate(true);
    }
}

void HsiWorkspace::updateWhiteLabel()
{
    m_whiteLabel->setText(m_whiteRef.isEmpty() ? tr("No white area chosen.")
                                               : tr("White area: rows %1–%2, columns %3–%4")
                                                     .arg(m_whiteRef.top() + 1).arg(m_whiteRef.bottom() + 1)
                                                     .arg(m_whiteRef.left() + 1).arg(m_whiteRef.right() + 1));
    m_clearWhite->setEnabled(!m_whiteRef.isEmpty());
}

void HsiWorkspace::clearWhiteReference()
{
    m_whiteRef = QRect();
    updateWhiteLabel();
    const bool wasOn = m_reflectance->isChecked();
    {
        const QSignalBlocker block(m_reflectance);
        m_reflectance->setChecked(false);
    }
    updateOverlays();
    if (wasOn)
        scheduleUpdate(false);
}

void HsiWorkspace::updateLayoutLabel()
{
    m_layoutLabel->setText(tr("Layout: %1").arg(MosaicLayoutDialog::describe(m_layout)));
    m_layoutLabel->setToolTip(QStringLiteral("[%1]").arg(m_layout.toString()));
}

void HsiWorkspace::editMosaicLayout()
{
    MosaicLayoutDialog dialog(m_layout, this);
    if (dialog.exec() != QDialog::Accepted || dialog.layout() == m_layout)
        return;
    ExtractOptions o = extractOptions();
    o.layout = dialog.layout();
    setExtractOptions(o);
}

ProcessingSettings HsiWorkspace::processingSettings() const
{
    ProcessingSettings s;
    s.reflectance = m_reflectance->isChecked();
    s.whiteReference = m_whiteRef;
    s.whiteReflectance = m_whiteValue->value();
    s.mnf = m_mnf->isChecked();
    s.mnfKeep = m_mnfKeep->value();
    s.guided = m_guided->isChecked();
    s.guidedRadius = m_guidedRadius->value();
    s.guidedEps = m_guidedEps->value();
    s.antiAlias = m_antiAlias->isChecked();
    s.antiAliasSigma = m_antiAliasSigma->value();
    s.upscale = m_upscale->currentData().toInt();
    s.interpolation = Interpolation(m_interpolation->currentIndex());
    s.absorbance = m_absorbance->isChecked();
    s.normalisation = SpectralNormalisation(m_normalisation->currentIndex());
    // "Smooth" is a derivative of order 0 with the filter on.
    s.smooth = m_derivative->currentIndex() > 0;
    s.derivative = std::max(0, m_derivative->currentIndex() - 1);
    s.sgWindow = m_sgWindow->value();
    s.sgPolynomial = m_sgPolynomial->value();
    return s;
}

void HsiWorkspace::setProcessingSettings(const ProcessingSettings& s)
{
    const QSignalBlocker b1(m_reflectance), b2(m_whiteValue), b3(m_mnf), b4(m_mnfKeep), b5(m_guided), b6(m_guidedRadius),
        b7(m_guidedEps), b8(m_antiAlias), b9(m_antiAliasSigma), b10(m_upscale), b11(m_interpolation), b12(m_absorbance),
        b13(m_normalisation), b14(m_derivative), b15(m_sgWindow), b16(m_sgPolynomial);
    m_reflectance->setChecked(s.reflectance);
    m_whiteRef = s.whiteReference;
    m_whiteValue->setValue(s.whiteReflectance);
    m_mnf->setChecked(s.mnf);
    m_mnfKeep->setValue(s.mnfKeep);
    m_guided->setChecked(s.guided);
    m_guidedRadius->setValue(s.guidedRadius);
    m_guidedEps->setValue(s.guidedEps);
    m_antiAlias->setChecked(s.antiAlias);
    m_antiAliasSigma->setValue(s.antiAliasSigma);
    m_upscale->setCurrentIndex(std::max(0, m_upscale->findData(s.upscale)));
    m_interpolation->setCurrentIndex(int(s.interpolation));
    m_absorbance->setChecked(s.absorbance);
    m_normalisation->setCurrentIndex(int(s.normalisation));
    m_derivative->setCurrentIndex(s.smooth || s.derivative > 0 ? s.derivative + 1 : 0);
    m_sgWindow->setValue(s.sgWindow);
    m_sgPolynomial->setValue(s.sgPolynomial);
    m_sgWindow->setEnabled(m_derivative->currentIndex() > 0);
    m_sgPolynomial->setEnabled(m_derivative->currentIndex() > 0);
    updateWhiteLabel();
    updateOverlays();
    scheduleUpdate(false);
}

void HsiWorkspace::scheduleUpdate(bool reextract)
{
    if (reextract && m_fromMosaic)
        m_needExtract = true;
    if (!m_fromMosaic && m_cubeExtracted.isNull())
        return;
    if (m_fromMosaic && m_mosaic.isNull())
        return;
    m_updateTimer->start();
    updateBusy();
}

void HsiWorkspace::startJob()
{
    if (m_running) {
        m_rerun = true;
        return;
    }
    const bool extractNow = m_needExtract && m_fromMosaic && !m_mosaic.isNull();
    const bool correctNow = m_needCorrect || extractNow || m_cubeRaw.isNull();
    const ExtractOptions eo = extractOptions();
    if (extractNow && !eo.layout.isValid()) {
        report(tr("The mosaic layout is not valid: every channel 1–16 must appear exactly once."));
        return;
    }
    m_needExtract = false;
    m_needCorrect = false;
    const Image2D mosaic = m_mosaic;
    const Cube baseExtracted = m_cubeExtracted;
    const Cube baseRaw = m_cubeRaw;
    const ReferenceFrames frames = m_frames;
    const SpectralCorrection correction = m_correction;
    const ProcessingSettings ps = processingSettings();
    const QVector<double> wl = wavelengths();
    m_running = true;
    m_watcher->setFuture(QtConcurrent::run([=] {
        JobResult r;
        if (extractNow) {
            Image2D m = mosaic;
            ExtractOptions o = eo;
            bool reflectance = false;
            if (frames.any()) {
                // Frames and image in the same units first (MSB-aligned data shifted down).
                auto scaled = [&](const Image2D& img) {
                    const double f = msbScale(img, eo.bitDepth);
                    if (f == 1.0 || img.isNull())
                        return img;
                    Image2D out(img.width(), img.height());
                    for (qsizetype i = 0; i < img.pixelCount(); ++i)
                        out.data()[i] = img.constData()[i] * f;
                    return out;
                };
                ReferenceFrames f = frames;
                f.dark = scaled(frames.dark);
                f.white = scaled(frames.white);
                const FrameCorrection fc = applyReferenceFrames(scaled(mosaic), f);
                if (!fc.ok()) {
                    r.error = fc.error;
                    return r;
                }
                m = fc.image;
                reflectance = fc.reflectance;
                r.notes << fc.notes;
                o.bitDepth = 0;
                if (!frames.dark.isNull())
                    o.blackLevel = 0.0;  // the dark frame replaces the black level
            }
            const ExtractResult e = extract(m, o);
            if (!e.ok()) {
                r.error = e.error;
                return r;
            }
            r.extractedCube = e.cube;
            if (reflectance)
                r.extractedCube.units = QStringLiteral("reflectance");
            if (frames.any())
                r.extractedCube.history.prepend(r.notes.join(QLatin1Char(' ')));
            r.notes << e.notes;
            r.extracted = true;
        } else {
            r.extractedCube = baseExtracted;
        }
        if (correctNow) {
            r.raw = correction.isValid() && correction.inBands == r.extractedCube.bands()
                        ? applyCorrection(r.extractedCube, correction)
                        : r.extractedCube;
        } else {
            r.raw = baseRaw;
        }
        if (wl.size() == r.raw.bands())
            r.raw.wavelengths = wl;
        r.processed = process(r.raw, ps);
        return r;
    }));
}

void HsiWorkspace::onJobFinished()
{
    const JobResult r = m_watcher->result();
    m_running = false;
    if (!r.error.isEmpty()) {
        report(r.error);
    } else {
        const bool sizeChanged = r.extracted || r.raw.rows() != m_cubeRaw.rows() || r.raw.cols() != m_cubeRaw.cols()
                                 || r.raw.bands() != m_cubeRaw.bands() || r.processed.rows() != m_cube.rows()
                                 || r.processed.cols() != m_cube.cols();
        if (r.extracted)
            m_cubeLayout = extractOptions().layout;
        m_cubeExtracted = r.extractedCube;
        m_cubeRaw = r.raw;
        m_cube = r.processed;
        if (!r.notes.isEmpty())
            report(r.notes.join(QLatin1Char(' ')));
        cubeReplaced(sizeChanged);
    }
    if (m_rerun) {
        m_rerun = false;
        startJob();
    }
    updateBusy();
}

void HsiWorkspace::showBusyIndicator(bool on)
{
    if (on) {
        m_busyOverlay->adjustSize();
        m_busyOverlay->move((m_viewGrid->width() - m_busyOverlay->width()) / 2, (m_viewGrid->height() - m_busyOverlay->height()) / 2);
        m_busyOverlay->raise();
    }
    m_busyOverlay->setVisible(on);
    if (on)
        setCursor(Qt::BusyCursor);
    else
        unsetCursor();
}

bool HsiWorkspace::busyIndicatorVisible() const
{
    return m_busyOverlay->isVisible();
}

bool HsiWorkspace::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_viewGrid && event->type() == QEvent::Resize && m_busyOverlay->isVisible())
        showBusyIndicator(true);  // keep it centred
    if (event->type() == QEvent::MouseButtonPress && m_paneCount > 1)
        for (int i = 0; i < m_paneCount; ++i)
            if (watched == m_panes[size_t(i)].header || watched == m_panes[size_t(i)].view->viewport()) {
                setActivePane(i);
                break;  // the press still goes on to the view
            }
    return QWidget::eventFilter(watched, event);
}

bool HsiWorkspace::isBusy() const
{
    return m_running || m_updateTimer->isActive() || m_clusterWatcher->isRunning();
}

void HsiWorkspace::updateBusy()
{
    const bool busy = isBusy();
    if (busy && !m_busyOverlay->isVisible() && !m_busyDelay->isActive())
        m_busyDelay->start();
    if (!busy) {
        m_busyDelay->stop();
        showBusyIndicator(false);
    }
    if (busy != m_busyReported) {
        m_busyReported = busy;
        emit busyChanged(busy);
    }
}

void HsiWorkspace::cubeReplaced(bool sizeOrSourceChanged)
{
    invalidateColour();
    invalidateAnalysis();
    m_scoreXIndex = m_scoreYIndex = -1;
    m_scorePlot->setSelection(QPolygonF());  // drawn on the old scores; the pixels stay selected
    if (sizeOrSourceChanged || m_selectionMask.size() != m_cube.planeSize()) {
        m_clustering = Clustering();
        m_clusterLabel->clear();
        m_similarMask.clear();
        m_selectionMask.clear();
    }
    // Areas outside a smaller cube are dropped.
    const QRect bounds(0, 0, m_cubeRaw.cols(), m_cubeRaw.rows());
    for (int i = int(m_rois.size()) - 1; i >= 0; --i)
        if (!bounds.contains(m_rois[i].rect))
            m_rois.removeAt(i);
    for (QRect* r : {&m_whiteRef, &m_colourRef, &m_similarityArea})
        if (!bounds.contains(*r))
            *r = QRect();
    updateWhiteLabel();
    updateBandControls();
    updateCubeInfo();
    m_plot->setWavelengths(m_cube.hasWavelengths() ? m_cube.wavelengths : QVector<double>());
    m_plot->setYLabel(unitsLabel(m_cube.units));
    renderView();
    updateSpectra();
    updateScores();
    updateSelectionInfo();
    emit cubeChanged();
}

void HsiWorkspace::updateCubeInfo()
{
    if (m_cubeRaw.isNull()) {
        m_cubeInfo->setText(tr("No cube yet."));
        return;
    }
    QString t = tr("Cube: %1 × %2 × %3 bands").arg(m_cubeRaw.rows()).arg(m_cubeRaw.cols()).arg(m_cubeRaw.bands());
    if (m_cube.rows() != m_cubeRaw.rows())
        t += tr(", %1 × %2 after processing").arg(m_cube.rows()).arg(m_cube.cols());
    if (m_cube.hasWavelengths())
        t += tr(", %1–%2 nm").arg(m_cube.wavelengths.first(), 0, 'f', 0).arg(m_cube.wavelengths.last(), 0, 'f', 0);
    m_cubeInfo->setText(t);
}

QVector<double> HsiWorkspace::wavelengths() const
{
    const int measured = m_cubeExtracted.isNull() ? kBands : m_cubeExtracted.bands();
    if (m_correction.isValid() && m_correction.inBands == measured) {
        if (m_correction.centres.size() == m_correction.outBands)
            return m_correction.centres;
        const SpectralResponse r = effectiveResponse();
        return r.isNull() ? QVector<double>() : r.peaks();
    }
    if (!m_response.isNull() && m_response.bands() == measured)
        return m_response.peaks();
    return m_cubeExtracted.wavelengths;
}

SpectralResponse HsiWorkspace::effectiveResponse() const
{
    if (m_correction.isValid() && !m_response.isNull() && m_response.bands() == m_correction.inBands)
        return correctedResponse(m_response, m_correction);
    return m_response;
}

void HsiWorkspace::applyWavelengths()
{
    const QVector<double> wl = wavelengths();
    if (wl.size() == m_cubeRaw.bands() && !m_cubeRaw.isNull()) {
        m_cubeRaw.wavelengths = wl;
        m_cube.wavelengths = wl;
    }
    updateBandControls();
    updateCubeInfo();
    m_plot->setWavelengths(m_cube.hasWavelengths() ? m_cube.wavelengths : QVector<double>());
    if (!m_indexFormula->text().isEmpty())
        setIndexFormula(m_indexFormula->text());
}

void HsiWorkspace::updateBandControls()
{
    const int bands = m_cube.isNull() ? kBands : m_cube.bands();
    m_bandSlider->setMaximum(bands - 1);
    const int b = m_bandSlider->value();
    m_bandLabel->setText(m_cube.isNull() ? tr("Band %1").arg(b + 1) : m_cube.bandLabel(b));
    for (QComboBox* c : m_rgbBands) {
        const int keep = c->currentIndex();
        const QSignalBlocker block(c);
        c->clear();
        for (int i = 0; i < bands; ++i)
            c->addItem(m_cube.isNull() ? tr("Band %1").arg(i + 1) : m_cube.bandLabel(i));
        if (keep >= 0 && keep < bands)
            c->setCurrentIndex(keep);
    }
    if (m_rgbBands[0]->currentIndex() == 0 && m_rgbBands[1]->currentIndex() == 0 && m_rgbBands[2]->currentIndex() == 0
        && !m_cube.isNull()) {
        int r, g, bl;
        pseudoTrueColourBands(m_cube, r, g, bl);
        const QSignalBlocker b0(m_rgbBands[0]), b1(m_rgbBands[1]), b2(m_rgbBands[2]);
        m_rgbBands[0]->setCurrentIndex(r);
        m_rgbBands[1]->setCurrentIndex(g);
        m_rgbBands[2]->setCurrentIndex(bl);
    }
}

// ---------------------------------------------------------------------------
// Colour
// ---------------------------------------------------------------------------

void HsiWorkspace::loadResponseDialog()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Load spectral response"), lastDir(),
                                                      tr("Spectral response (*.mat *.csv *.txt);;All files (*)"));
    if (!path.isEmpty())
        loadResponse(path);
}

bool HsiWorkspace::loadResponse(const QString& path)
{
    rememberDir(path);
    const ResponseFile f = hsi::loadResponse(path);
    if (!f.ok()) {
        report(f.error);
        return false;
    }
    m_response = f.response;
    m_directRgb = f.rgbMatrix;
    m_responseName = QFileInfo(path).fileName();
    m_responsePath = QFileInfo(path).absoluteFilePath();
    m_responseNotes = f.notes.join(QLatin1Char(' '));
    m_xyz.clear();
    invalidateColour();
    updateCorrection();
    updateCorrectionStatus();
    for (QWidget* w : {static_cast<QWidget*>(m_sigma), static_cast<QWidget*>(m_alpha), static_cast<QWidget*>(m_wb),
                       static_cast<QWidget*>(m_pickColourRef)})
        w->setEnabled(f.hasResponse());
    applyWavelengths();
    if (m_correction.isValid())
        scheduleCorrection();
    report(tr("Spectral response loaded: %1").arg(m_responseName));
    if (!m_cube.isNull())
        setViewMode(ViewMode::Colour);
    else
        renderView();
    updateSpectra();
    return true;
}

bool HsiWorkspace::hasColour() const
{
    return (!m_response.isNull() && effectiveResponse().bands() == m_cube.bands())
           || (m_directRgb.size() == 3 * m_cube.bands() && !m_cube.isNull());
}

ColourSettings HsiWorkspace::colourSettings() const
{
    ColourSettings s;
    s.sigmaNm = m_sigma->value();
    s.alphaRel = m_alpha->value();
    s.whiteBalance = WhiteBalance(m_wb->currentIndex());
    s.reference = toProcessed(m_colourRef);
    s.exposurePercentile = m_exposure->value();
    return s;
}

void HsiWorkspace::setColourSettings(const ColourSettings& s)
{
    const QSignalBlocker b1(m_sigma), b2(m_alpha), b3(m_wb), b4(m_exposure);
    m_sigma->setValue(s.sigmaNm);
    m_alpha->setValue(s.alphaRel);
    m_wb->setCurrentIndex(int(s.whiteBalance));
    m_exposure->setValue(s.exposurePercentile);
    m_xyz.clear();
    invalidateColour();
    renderView();
}

void HsiWorkspace::invalidateColour()
{
    m_colourDirty = true;
}

QVector<double> HsiWorkspace::colourMatrix()
{
    if (!hasColour())
        return {};
    if (!m_colourDirty)
        return m_rgb;
    m_colourDirty = false;
    if (m_directRgb.size() == 3 * m_cube.bands() && m_response.isNull()) {
        m_rgb = m_directRgb;
        return m_rgb;
    }
    const SpectralResponse response = effectiveResponse();
    if (m_xyz.isEmpty())
        m_xyz = xyzMatrix(response, m_sigma->value(), m_alpha->value());
    ColourSettings cs = colourSettings();
    if (cs.whiteBalance == WhiteBalance::Reference && cs.reference.isEmpty()) {
        report(tr("Choose a reference area for the white balance (Pick reference area); grey world is used until then."));
        cs.whiteBalance = WhiteBalance::GrayWorld;
    }
    m_rgb = rgbMatrix(m_cube, m_xyz, cs, response);
    if (m_rgb.isEmpty())
        report(tr("The colour matrix could not be computed (singular matrix or a black reference area)."));
    return m_rgb;
}

// ---------------------------------------------------------------------------
// View
// ---------------------------------------------------------------------------

void HsiWorkspace::setViewMode(ViewMode mode)
{
    m_viewMode->setCurrentIndex(int(mode));
}

HsiWorkspace::ViewMode HsiWorkspace::viewMode() const
{
    return m_panes[size_t(m_activePane)].viewMode;
}

void HsiWorkspace::setActivePane(int pane)
{
    pane = std::clamp(pane, 0, m_paneCount - 1);
    const bool changed = pane != m_activePane;
    m_activePane = pane;
    Pane& p = m_panes[size_t(pane)];
    {
        const QSignalBlocker block(m_viewMode);
        m_viewMode->setCurrentIndex(int(p.viewMode));
    }
    m_viewOptions->setCurrentIndex(int(p.viewMode));
    {
        const QSignalBlocker block(m_bandSlider);
        m_bandSlider->setValue(p.band);
    }
    if (p.rgb[0] >= 0) {
        for (int c = 0; c < 3; ++c) {
            const QSignalBlocker block(m_rgbBands[size_t(c)]);
            if (p.rgb[size_t(c)] < m_rgbBands[size_t(c)]->count())
                m_rgbBands[size_t(c)]->setCurrentIndex(p.rgb[size_t(c)]);
        }
    } else {
        for (int c = 0; c < 3; ++c)
            p.rgb[size_t(c)] = m_rgbBands[size_t(c)]->currentIndex();
    }
    updateBandControls();
    updatePaneTitles();
    m_shown = p.shown;
    if (changed) {
        updateSpectra();
        emit activePaneChanged(pane);
    }
}

void HsiWorkspace::updatePaneTitles()
{
    for (int i = 0; i < kMaxPanes; ++i) {
        Pane& p = m_panes[size_t(i)];
        QString text = QStringLiteral("%1 · %2").arg(i + 1).arg(m_viewMode->itemText(int(p.viewMode)));
        if (p.viewMode == ViewMode::Band && !m_cube.isNull())
            text += QStringLiteral(" · ") + m_cube.bandLabel(std::min(p.band, m_cube.bands() - 1));
        const bool active = i == m_activePane && m_paneCount > 1;
        p.title->setText(active ? QStringLiteral("<b>%1</b>").arg(text.toHtmlEscaped()) : text.toHtmlEscaped());
        p.box->setStyleSheet(active ? QStringLiteral("QFrame#%1 { border: 2px solid palette(highlight); border-radius: 3px; }")
                                          .arg(p.box->objectName())
                                    : QStringLiteral("QFrame#%1 { border: 2px solid transparent; }").arg(p.box->objectName()));
    }
}

void HsiWorkspace::setBand(int band)
{
    m_bandSlider->setValue(band);
}

int HsiWorkspace::band() const
{
    return m_bandSlider->value();
}

int HsiWorkspace::renderBand() const
{
    return m_renderPane >= 0 ? m_panes[size_t(m_renderPane)].band : band();
}

std::array<int, 3> HsiWorkspace::paneRgbBands() const
{
    if (m_renderPane >= 0 && m_panes[size_t(m_renderPane)].rgb[0] >= 0)
        return m_panes[size_t(m_renderPane)].rgb;
    return {m_rgbBands[0]->currentIndex(), m_rgbBands[1]->currentIndex(), m_rgbBands[2]->currentIndex()};
}

void HsiWorkspace::setFalseColourBands(int r, int g, int b)
{
    {
        const QSignalBlocker b0(m_rgbBands[0]), b1(m_rgbBands[1]), b2(m_rgbBands[2]);
        m_rgbBands[0]->setCurrentIndex(r);
        m_rgbBands[1]->setCurrentIndex(g);
        m_rgbBands[2]->setCurrentIndex(b);
    }
    if (viewMode() == ViewMode::FalseColour)
        renderView();
}

void HsiWorkspace::setIndexFormula(const QString& formula)
{
    if (m_indexFormula->text() != formula)
        m_indexFormula->setText(formula);
    const int preset = m_indexPreset->findData(formula);
    {
        const QSignalBlocker block(m_indexPreset);
        m_indexPreset->setCurrentIndex(preset >= 0 ? preset : 0);
    }
    m_indexError.clear();
    m_index.reset();
    m_indexImage = Image2D();
    if (formula.trimmed().isEmpty()) {
        m_indexLabel->clear();
    } else {
        const int bands = m_cube.isNull() ? kBands : m_cube.bands();
        m_index = BandMath::parse(formula, bands, m_cube.hasWavelengths() ? m_cube.wavelengths : QVector<double>(), &m_indexError);
        if (!m_index)
            m_indexLabel->setText(m_indexError);
    }
    if (viewMode() == ViewMode::Index)
        renderView();
    else
        setViewMode(ViewMode::Index);
}

void HsiWorkspace::setTool(Tool tool)
{
    if (tool != Tool::Pan && tool != Tool::Spectrum && m_tool != tool)
        m_previousTool = (m_tool == Tool::Pan || m_tool == Tool::Spectrum) ? m_tool : m_previousTool;
    m_tool = tool;
    {
        const QSignalBlocker block(m_toolCombo);
        m_toolCombo->setCurrentIndex(int(tool));
    }
    for (Pane& pane : m_panes)
        pane.view->setRegionTool(tool != Tool::Pan);
    updateOverlays();
    switch (tool) {
    case Tool::Pan: break;
    case Tool::Spectrum: report(tr("Click a pixel or drag an area to keep its spectrum.")); break;
    case Tool::WhiteReference: report(tr("Drag a rectangle over the white reference.")); break;
    case Tool::ColourReference: report(tr("Drag a rectangle over a white or grey area for the white balance.")); break;
    case Tool::SimilarityReference: report(tr("Click a pixel or drag an area: similar areas will be marked.")); break;
    }
}

void HsiWorkspace::setAbCompare(bool on)
{
    m_abButton->setChecked(on);
}

bool HsiWorkspace::abCompare() const
{
    return m_abButton->isChecked();
}

double HsiWorkspace::processedScale() const
{
    return m_cubeRaw.cols() > 0 ? double(m_cube.cols()) / m_cubeRaw.cols() : 1.0;
}

double HsiWorkspace::displayScale(int pane) const
{
    const QImage& shown = m_panes[size_t(pane)].shown;
    return m_cubeRaw.cols() > 0 && !shown.isNull() ? double(shown.width()) / m_cubeRaw.cols() : 1.0;
}

QRect HsiWorkspace::toProcessed(const QRect& r) const
{
    if (r.isEmpty())
        return {};
    const double f = processedScale();
    return QRect(int(std::floor(r.left() * f)), int(std::floor(r.top() * f)), std::max(1, int(std::lround(r.width() * f))),
                 std::max(1, int(std::lround(r.height() * f))));
}

QRect HsiWorkspace::toCube(const QRect& r, const Cube& cube) const
{
    if (r.isEmpty() || m_cubeRaw.cols() == 0)
        return {};
    const double f = double(cube.cols()) / m_cubeRaw.cols();
    return QRect(int(std::floor(r.left() * f)), int(std::floor(r.top() * f)), std::max(1, int(std::lround(r.width() * f))),
                 std::max(1, int(std::lround(r.height() * f))));
}

void HsiWorkspace::invalidateAnalysis()
{
    m_pca = Pca();
    m_indexImage = Image2D();
    m_match = AreaMatch();
    if (!m_indexFormula->text().isEmpty()) {
        const int bands = m_cube.isNull() ? kBands : m_cube.bands();
        m_index = BandMath::parse(m_indexFormula->text(), bands,
                                  m_cube.hasWavelengths() ? m_cube.wavelengths : QVector<double>(), &m_indexError);
    }
}

void HsiWorkspace::fillViewModes(QComboBox* combo)
{
    combo->addItem(tr("Raw mosaic"));
    combo->addItem(tr("Band"));
    combo->addItem(tr("All bands"));
    combo->addItem(tr("Colour (sRGB)"));
    combo->addItem(tr("False colour"));
    combo->addItem(tr("Principal components"));
    combo->addItem(tr("Spectral index"));
    combo->addItem(tr("Similar areas (spectral angle)"));
    combo->addItem(tr("Classes (k-means)"));
    combo->addItem(tr("Match to drawn areas"));
    combo->setItemData(int(ViewMode::Matches),
                       tr("Every pixel in the colour of the drawn area whose spectrum it resembles most (spectral angle "
                          "mapper classification), darker the less it resembles it"),
                       Qt::ToolTipRole);
}

HsiWorkspace::ViewMode HsiWorkspace::paneMode(int pane) const
{
    return m_panes[size_t(pane)].viewMode;
}

bool HsiWorkspace::anyPaneShows(ViewMode mode) const
{
    for (int i = 0; i < m_paneCount; ++i)
        if (paneMode(i) == mode)
            return true;
    return false;
}

void HsiWorkspace::setPaneCount(int count)
{
    count = count >= 4 ? 4 : count >= 2 ? 2 : 1;
    {
        const QSignalBlocker block(m_paneCountCombo);
        m_paneCountCombo->setCurrentIndex(count == 1 ? 0 : count == 2 ? 1 : 2);
    }
    if (count == m_paneCount)
        return;
    m_paneCount = count;
    for (Pane& pane : m_panes)
        m_gridLayout->removeWidget(pane.box);
    updateGridStretch();
    for (int i = 0; i < kMaxPanes; ++i) {
        Pane& pane = m_panes[size_t(i)];
        const bool on = i < count;
        pane.box->setVisible(on);
        pane.header->setVisible(count > 1);
        if (on)
            m_gridLayout->addWidget(pane.box, i / 2, i % 2);
        else {
            pane.view->clearImage();
            pane.shown = QImage();
        }
    }
    if (m_activePane >= count)
        setActivePane(0);
    updatePaneTitles();
    renderView();
    updateSpectra();
}

QImage HsiWorkspace::paneImage(int pane) const
{
    return m_panes[size_t(pane)].shown;
}

void HsiWorkspace::updateGridStretch()
{
    // Only the rows and columns in use take space: one view fills the whole area.
    const int cols = m_paneCount > 1 ? 2 : 1, rows = m_paneCount > 2 ? 2 : 1;
    for (int k = 0; k < 2; ++k) {
        m_gridLayout->setColumnStretch(k, k < cols ? 1 : 0);
        m_gridLayout->setRowStretch(k, k < rows ? 1 : 0);
    }
}

void HsiWorkspace::resetPane(int i, bool render)
{
    Pane& p = m_panes[size_t(i)];
    p.viewMode = kDefaultPaneModes[size_t(i)];
    p.band = 0;
    p.rgb = {-1, -1, -1};
    if (!m_cube.isNull()) {
        int r, g, b;
        pseudoTrueColourBands(m_cube, r, g, b);
        p.rgb = {r, g, b};
    }
    if (i == m_activePane)
        setActivePane(i);  // the controls show the reset settings
    p.view->fitToWindow();
    if (render) {
        updatePaneTitles();
        renderView();
        updateSpectra();
    }
}

void HsiWorkspace::refreshPane(int i)
{
    // Results the view's mode keeps between draws are computed again.
    switch (paneMode(i)) {
    case ViewMode::Colour:
        m_xyz.clear();
        invalidateColour();
        break;
    case ViewMode::Pca:
        m_pca = Pca();
        m_scoreXIndex = m_scoreYIndex = -1;
        break;
    case ViewMode::Index:
        m_indexImage = Image2D();
        break;
    case ViewMode::Matches:
        m_match = AreaMatch();
        break;
    default:
        break;
    }
    renderView();
    updateSpectra();
    if (paneMode(i) == ViewMode::Pca)
        updateScores();
}

void HsiWorkspace::refreshAll()
{
    m_xyz.clear();
    invalidateColour();
    invalidateAnalysis();
    m_needCorrect = true;
    if (m_fromMosaic)
        m_needExtract = true;
    if (m_cubeExtracted.isNull() && m_mosaic.isNull())
        return;
    if (m_running) {
        m_rerun = true;
        return;
    }
    m_updateTimer->stop();
    startJob();
    updateBusy();
}

void HsiWorkspace::resetAll()
{
    m_abButton->setChecked(false);
    clearScoreSelection();
    setColourSettings(ColourSettings());
    m_colourRef = QRect();
    for (int i = 0; i < kMaxPanes; ++i)
        resetPane(i, false);
    setActivePane(0);
    // Turning processing off makes the cube again.
    const bool wasOn = processingSettings().any();
    setProcessingSettings(ProcessingSettings());  // also removes the white area
    if (!wasOn)
        renderView();
    updatePaneTitles();
    updateOverlays();
    updateSpectra();
    report(tr("Processing and views reset to their defaults."));
}

void HsiWorkspace::setPaneMode(int pane, ViewMode mode)
{
    if (pane == m_activePane) {
        setViewMode(mode);
        return;
    }
    m_panes[size_t(pane)].viewMode = mode;
    updatePaneTitles();
    renderView();
    updateSpectra();
}

QImage HsiWorkspace::renderMode(ViewMode mode, const Cube& cube, bool forCompare, Stretch* st)
{
    // A/B: the "before" picture reuses the stretch of the "after" one where
    // the units are the same, so the two sides can be compared directly.
    const bool useGiven = forCompare && st && st->valid;
    auto greyOf = [](const Cube& c) {
        const Image2D grey = c.meanImage();
        const auto gr = autoRange(grey);
        return renderScalar(grey, gr.first, gr.second);
    };
    switch (mode) {
    case ViewMode::Mosaic: {
        if (m_mosaic.isNull())
            return {};
        const auto r = autoRange(m_mosaic);
        return renderScalar(m_mosaic, r.first, r.second);
    }
    case ViewMode::Band: {
        const Image2D b = cube.bandImage(std::min(renderBand(), cube.bands() - 1));
        const Range r = useGiven ? st->ranges[0] : autoRange(b);
        if (st && !useGiven)
            st->ranges[0] = r;
        return renderScalar(b, r.first, r.second);
    }
    case ViewMode::Gallery: {
        Range r = st ? st->ranges[0] : Range();
        const QImage img = renderBandGallery(cube, 2, &r, useGiven);
        if (st && !useGiven)
            st->ranges[0] = r;
        return img;
    }
    case ViewMode::Colour: {
        const QVector<double> m = colourMatrix();
        if (m.isEmpty()) {
            int r, g, b;
            pseudoTrueColourBands(cube, r, g, b);
            std::array<Range, 3> ranges = st ? st->ranges : std::array<Range, 3>();
            const QImage img = renderComposite(cube.bandImage(r), cube.bandImage(g), cube.bandImage(b), ranges, useGiven);
            if (st && !useGiven)
                st->ranges = ranges;
            return img;
        }
        double scale = useGiven ? st->rgbScale : 0.0;
        const QImage img = renderRgb(cube, m, m_exposure->value(), &scale);
        if (st && !useGiven)
            st->rgbScale = scale;
        return img;
    }
    case ViewMode::FalseColour: {
        const int bands = cube.bands();
        const std::array<int, 3> rgb = paneRgbBands();
        const int r = std::min(rgb[0], bands - 1), g = std::min(rgb[1], bands - 1), b = std::min(rgb[2], bands - 1);
        if (r < 0 || g < 0 || b < 0)
            return {};
        std::array<Range, 3> ranges = st ? st->ranges : std::array<Range, 3>();
        const QImage img = renderComposite(cube.bandImage(r), cube.bandImage(g), cube.bandImage(b), ranges, useGiven);
        if (st && !useGiven)
            st->ranges = ranges;
        return img;
    }
    case ViewMode::Pca: {
        if (!m_pca.isValid())
            m_pca = principalComponents(m_cube);
        if (!m_pca.isValid() || cube.bands() < 3)
            return {};
        if (!forCompare) {
            const QVector<double> e = m_pca.explained();
            QStringList parts;
            for (int i = 0; i < std::min<qsizetype>(5, e.size()); ++i)
                parts << QStringLiteral("PC%1 %2 %").arg(i + 1).arg(100.0 * e[i], 0, 'f', 1);
            m_pcaLabel->setText(tr("Red, green, blue = first three principal components. Variance: %1")
                                    .arg(parts.join(QStringLiteral(", "))));
        }
        std::array<Range, 3> ranges = st ? st->ranges : std::array<Range, 3>();
        const QImage img = renderComposite(pcaComponent(cube, m_pca, 0), pcaComponent(cube, m_pca, 1),
                                           pcaComponent(cube, m_pca, 2), ranges, useGiven);
        if (st && !useGiven)
            st->ranges = ranges;
        return img;
    }
    case ViewMode::Index: {
        if (!m_index) {
            // No (valid) formula yet: the mean of all bands, and a hint.
            if (!forCompare && m_indexError.isEmpty())
                m_indexLabel->setText(tr("Choose a preset or type a formula, e.g. (b7 - b11) / (b7 + b11)."));
            return greyOf(cube);
        }
        const Image2D img = m_index->evaluate(cube);
        if (img.isNull())
            return {};
        const Range r = useGiven ? st->ranges[0] : autoRange(img, 0.02, 0.98);
        if (st && !useGiven)
            st->ranges[0] = r;
        if (!forCompare) {
            m_indexImage = img;
            m_indexLabel->setText(tr("%1 · display range %2 … %3 (2–98 %)")
                                      .arg(m_index->resolved()).arg(r.first, 0, 'g', 4).arg(r.second, 0, 'g', 4));
        }
        return renderScalar(img, r.first, r.second, Colormap(m_colormap->currentIndex()));
    }
    case ViewMode::Similarity: {
        const QImage base = greyOf(cube);
        if (m_similarityRef.size() != cube.bands())
            return base;
        const Image2D angle = spectralAngleMap(cube, m_similarityRef);
        const double thr = m_simThreshold->value() / kRadToDeg;
        QVector<quint8> mask(angle.pixelCount(), 0);
        qint64 n = 0;
        for (qsizetype i = 0; i < angle.pixelCount(); ++i)
            if (angle.constData()[i] <= thr) {
                mask[i] = 1;
                ++n;
            }
        if (!forCompare) {
            m_similarMask = mask;
            m_simLabel->setText(tr("Reference: %1 · %2 pixels (%3 %) within %4°")
                                    .arg(m_similarityName).arg(n).arg(100.0 * double(n) / double(std::max<qsizetype>(1, angle.pixelCount())), 0, 'f', 1)
                                    .arg(m_simThreshold->value()));
        }
        if (m_simDisplay->currentIndex() == 1) {
            Image2D deg(angle.width(), angle.height());
            for (qsizetype i = 0; i < angle.pixelCount(); ++i)
                deg.data()[i] = angle.constData()[i] * kRadToDeg;
            return renderScalar(deg, 0.0, std::max(1.0, 3.0 * m_simThreshold->value()), Colormap::Turbo);
        }
        return renderMaskOverlay(base, mask, QColor(0, 220, 255));
    }
    case ViewMode::Classes: {
        if (m_clustering.isValid() && m_clustering.labels.size() == cube.planeSize() && !forCompare)
            return renderClasses(m_clustering, cube.cols(), cube.rows());
        return greyOf(cube);
    }
    case ViewMode::Matches: {
        const QImage base = greyOf(cube);
        if (m_rois.isEmpty()) {
            if (!forCompare)
                m_matchLabel->setText(tr("Draw areas with the spectrum tool: every pixel takes the colour of the area it resembles most."));
            return base;
        }
        QVector<QVector<double>> refs;
        QVector<QColor> colours;
        for (const Roi& r : std::as_const(m_rois)) {
            const RegionStats st2 = regionStats(cube, toCube(r.rect, cube));
            refs << (st2.isValid() ? st2.mean : QVector<double>(cube.bands(), 0.0));
            colours << r.color;
        }
        const AreaMatch match = matchToReferences(cube, refs, m_matchAngle->value() / kRadToDeg);
        if (!forCompare) {
            m_match = match;
            QStringList parts;
            qint64 matched = 0;
            for (int k = 0; k < match.k; ++k) {
                parts << QStringLiteral("%1 %2 %").arg(m_rois[k].name).arg(100.0 * double(match.counts[k]) / double(std::max<qsizetype>(1, cube.planeSize())), 0, 'f', 1);
                matched += match.counts[k];
            }
            parts << tr("unmatched %1 %").arg(100.0 * double(cube.planeSize() - matched) / double(std::max<qsizetype>(1, cube.planeSize())), 0, 'f', 1);
            m_matchLabel->setText(parts.join(QStringLiteral(" · ")));
        }
        return renderAreaMatch(match, cube.cols(), cube.rows(), colours, m_matchShade->isChecked(),
                               m_matchBackground->currentIndex() == 0 ? base : QImage());
    }
    }
    return {};
}

void HsiWorkspace::updateAbButton()
{
    const bool can = !m_cubeRaw.isNull() && processingSettings().any();
    m_abButton->setEnabled(can);
    m_abButton->setToolTip(can ? tr("Compare with the unprocessed cube: drag the divider across the image")
                               : tr("A/B compares the processed cube with the unprocessed one: turn on a processing step first"));
}

void HsiWorkspace::renderView()
{
    updateAbButton();
    const bool nothing = m_cube.isNull() && m_mosaic.isNull();
    if (nothing) {
        for (Pane& pane : m_panes) {
            pane.shown = QImage();
            pane.view->clearImage();
        }
        m_shown = QImage();
        m_centre->setCurrentWidget(m_dropHint);
        return;
    }
    m_centre->setCurrentWidget(m_viewGrid);
    const bool compare = abCompare() && m_abButton->isEnabled();
    for (int i = 0; i < m_paneCount; ++i) {
        Pane& pane = m_panes[size_t(i)];
        const ViewMode mode = paneMode(i);
        Stretch st;
        m_renderPane = i;
        pane.shown = (m_cube.isNull() && mode != ViewMode::Mosaic) ? QImage() : renderMode(mode, m_cube, false, &st);
        if (pane.shown.isNull()) {
            pane.view->clearImage();
            continue;
        }
        if (mode != ViewMode::Mosaic && mode != ViewMode::Gallery)
            tintSelection(pane.shown);
        pane.view->setImage(pane.shown);
        // A/B: the same view of the unprocessed cube, at the size on display.
        if (compare && mode != ViewMode::Mosaic && mode != ViewMode::Classes && !m_cubeRaw.isNull()) {
            st.valid = m_cubeRaw.units == m_cube.units;
            QImage before = renderMode(mode, m_cubeRaw, true, &st);
            if (!before.isNull() && before.size() != pane.shown.size())
                before = before.scaled(pane.shown.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
            pane.view->setCompareImage(before);
        } else {
            pane.view->setCompareImage(QImage());
        }
    }
    m_renderPane = -1;
    m_shown = m_panes[size_t(m_activePane)].shown;
    updatePaneTitles();
    updateOverlays();
}

void HsiWorkspace::updateOverlays()
{
    for (int i = 0; i < kMaxPanes; ++i) {
        Pane& pane = m_panes[size_t(i)];
        QList<ViewOverlay> overlays;
        QList<OverlayRef> refs;
        if (i < m_paneCount && !pane.shown.isNull() && paneMode(i) != ViewMode::Gallery) {
            const double s = displayScale(i);
            auto scaled = [s](const QRect& r) { return QRectF(r.left() * s, r.top() * s, r.width() * s, r.height() * s); };
            for (int k = 0; k < m_rois.size(); ++k) {
                overlays << ViewOverlay{scaled(m_rois[k].rect), m_rois[k].color, m_rois[k].name, false, true};
                refs << OverlayRef{OverlayRef::Roi, k};
            }
            if (!m_whiteRef.isEmpty() && (m_reflectance->isChecked() || m_tool == Tool::WhiteReference)) {
                overlays << ViewOverlay{scaled(m_whiteRef), QColor(255, 255, 255), tr("White"), true, true};
                refs << OverlayRef{OverlayRef::White, 0};
            }
            if (!m_colourRef.isEmpty() && m_wb->currentIndex() == int(WhiteBalance::Reference)) {
                overlays << ViewOverlay{scaled(m_colourRef), QColor(255, 220, 60), tr("WB"), true, true};
                refs << OverlayRef{OverlayRef::ColourReference, 0};
            }
            if (!m_similarityArea.isEmpty() && paneMode(i) == ViewMode::Similarity) {
                overlays << ViewOverlay{scaled(m_similarityArea), QColor(0, 220, 255), tr("Reference"), true, true};
                refs << OverlayRef{OverlayRef::SimilarityArea, 0};
            }
        }
        pane.overlayRefs = refs;
        pane.view->setOverlays(overlays);
    }
}

void HsiWorkspace::removeRoi(int index)
{
    if (index < 0 || index >= m_rois.size())
        return;
    m_rois.removeAt(index);
    updateSpectra();
    if (anyPaneShows(ViewMode::Matches))
        renderView();
    else
        updateOverlays();
}

void HsiWorkspace::removeOverlay(int pane, int index)
{
    const QList<OverlayRef>& refs = m_panes[size_t(pane)].overlayRefs;
    if (index < 0 || index >= refs.size())
        return;
    const OverlayRef ref = refs[index];
    switch (ref.kind) {
    case OverlayRef::Roi:
        removeRoi(ref.index);
        break;
    case OverlayRef::White:
        clearWhiteReference();
        break;
    case OverlayRef::ColourReference:
        m_colourRef = QRect();
        m_wb->setCurrentIndex(int(WhiteBalance::GrayWorld));  // redraws
        updateOverlays();
        break;
    case OverlayRef::SimilarityArea:
        m_similarityArea = QRect();
        m_similarityRef.clear();
        m_similarMask.clear();
        m_simLabel->setText(tr("Choose a reference: click a pixel or drag an area."));
        renderView();
        updateSpectra();
        break;
    }
}

// ---------------------------------------------------------------------------
// Views in step
// ---------------------------------------------------------------------------

// Pixels on display per pixel of the processed cube, in one view.
double HsiWorkspace::paneScale(int pane) const
{
    const QImage& shown = m_panes[size_t(pane)].shown;
    return m_cube.cols() > 0 && !shown.isNull() ? double(shown.width()) / m_cube.cols() : 0.0;
}

void HsiWorkspace::onPaneViewChanged(int pane)
{
    if (m_syncing || paneMode(pane) == ViewMode::Gallery)
        return;
    const double f = paneScale(pane);
    if (f <= 0.0)
        return;
    const ViewState s = m_panes[size_t(pane)].view->viewState();
    m_sharedView = ViewState{s.fit, s.zoom * f, s.center / f};
    m_haveSharedView = true;
    m_syncing = true;
    for (int j = 0; j < m_paneCount; ++j)
        if (j != pane)
            applySharedView(j);
    m_syncing = false;
}

void HsiWorkspace::applySharedView(int pane)
{
    if (!m_haveSharedView || pane >= m_paneCount || paneMode(pane) == ViewMode::Gallery)
        return;
    const double f = paneScale(pane);
    if (f <= 0.0)
        return;
    m_panes[size_t(pane)].view->setViewState(ViewState{m_sharedView.fit, m_sharedView.zoom / f, m_sharedView.center * f});
}

bool HsiWorkspace::toCubePixel(int pane, int x, int y, int& cx, int& cy) const
{
    const Pane& p = m_panes[size_t(pane)];
    if (m_cube.isNull() || p.shown.isNull() || x < 0 || y < 0)
        return false;
    if (paneMode(pane) == ViewMode::Gallery) {
        const int gridCols = int(std::ceil(std::sqrt(double(m_cube.bands()))));
        const int tileW = m_cube.cols() + 2, tileH = m_cube.rows() + 2;
        cx = x % tileW;
        cy = y % tileH;
        const int b = (y / tileH) * gridCols + x / tileW;
        return cx < m_cube.cols() && cy < m_cube.rows() && b < m_cube.bands();
    }
    cx = std::min(m_cube.cols() - 1, int(x * double(m_cube.cols()) / p.shown.width()));
    cy = std::min(m_cube.rows() - 1, int(y * double(m_cube.rows()) / p.shown.height()));
    return true;
}

void HsiWorkspace::updatePointers(int sourcePane, int cx, int cy)
{
    for (int j = 0; j < kMaxPanes; ++j) {
        Pane& pane = m_panes[size_t(j)];
        QList<QPointF> points;
        if (cx >= 0 && j < m_paneCount && !pane.shown.isNull()) {
            if (paneMode(j) == ViewMode::Gallery) {
                points = galleryPositions(m_cube, cx + 0.5, cy + 0.5);  // the same pixel in every band
            } else if (j != sourcePane) {
                const double f = paneScale(j);
                points << QPointF((cx + 0.5) * f, (cy + 0.5) * pane.shown.height() / double(m_cube.rows()));
            }
        }
        pane.view->setPointers(points);
    }
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

void HsiWorkspace::onHover(int pane, int x, int y)
{
    int cx = -1, cy = -1;
    if (!toCubePixel(pane, x, y, cx, cy)) {
        m_hoverSpectrum.clear();
        m_readout->clear();
        m_scorePlot->setMarker(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
        updatePointers(pane, -1, -1);
        updateSpectra();
        return;
    }
    const ViewMode mode = paneMode(pane);
    m_hoverSpectrum = m_cube.spectrum(cx, cy);
    if (!m_scoreXImage.isNull() && cx < m_scoreXImage.width() && cy < m_scoreXImage.height())
        m_scorePlot->setMarker(m_scoreXImage.at(cx, cy), m_scoreYImage.at(cx, cy));
    m_hoverName = tr("Pixel (%1, %2)").arg(cy + 1).arg(cx + 1);
    QString text = tr("Row %1, column %2").arg(cy + 1).arg(cx + 1);
    if (mode == ViewMode::Mosaic && x < m_mosaic.width() && y < m_mosaic.height())
        text = tr("Sensor row %1, column %2: %3 DN (channel %4)")
                   .arg(y + 1).arg(x + 1).arg(m_mosaic.at(x, y), 0, 'g', 6)
                   .arg(extractOptions().layout.at(y % kPattern, x % kPattern));
    else if (mode == ViewMode::Band) {
        const int b = std::min(m_panes[size_t(pane)].band, m_cube.bands() - 1);
        text += tr(" · band %1: %2").arg(b + 1).arg(m_cube.at(cx, cy, b), 0, 'g', 6);
    }
    else if (mode == ViewMode::Index && !m_indexImage.isNull() && m_indexImage.width() == m_cube.cols())
        text += tr(" · index: %1").arg(m_indexImage.at(cx, cy), 0, 'g', 5);
    else if (mode == ViewMode::Similarity && m_similarityRef.size() == m_cube.bands()) {
        double dot = 0.0, n1 = 0.0, n2 = 0.0;
        for (int b = 0; b < m_cube.bands(); ++b) {
            dot += m_hoverSpectrum[b] * m_similarityRef[b];
            n1 += m_hoverSpectrum[b] * m_hoverSpectrum[b];
            n2 += m_similarityRef[b] * m_similarityRef[b];
        }
        if (n1 > 0.0 && n2 > 0.0)
            text += tr(" · angle to reference: %1°").arg(std::acos(std::clamp(dot / std::sqrt(n1 * n2), -1.0, 1.0)) * kRadToDeg, 0, 'f', 2);
    } else if (mode == ViewMode::Classes && m_clustering.isValid() && m_clustering.labels.size() == m_cube.planeSize())
        text += tr(" · class %1").arg(m_clustering.labels[qsizetype(cy) * m_cube.cols() + cx] + 1);
    else if (mode == ViewMode::Matches && m_match.isValid() && m_match.best.size() == m_cube.planeSize()) {
        const qsizetype p = qsizetype(cy) * m_cube.cols() + cx;
        const int k = m_match.best[p];
        if (k >= 0 && k < m_rois.size())
            text += tr(" · matches %1 (%2°)").arg(m_rois[k].name).arg(double(m_match.angle[p]) * kRadToDeg, 0, 'f', 1);
        else
            text += tr(" · no match within %1°").arg(m_matchAngle->value());
    }
    m_readout->setText(text);
    updatePointers(pane, cx, cy);
    updateSpectra();
}

void HsiWorkspace::onPixelClicked(int pane, int x, int y)
{
    int cx, cy;
    if (!toCubePixel(pane, x, y, cx, cy))
        return;
    const double f = processedScale();
    handleSelection(QRect(int(cx / f), int(cy / f), 1, 1), true);
}

void HsiWorkspace::onRegionSelected(int pane, const QRect& d)
{
    const double s = displayScale(pane);
    if (s <= 0.0 || paneMode(pane) == ViewMode::Gallery)
        return;
    const QRect r = QRect(QPoint(int(std::floor(d.left() / s)), int(std::floor(d.top() / s))),
                          QPoint(int(std::ceil((d.right() + 1) / s)) - 1, int(std::ceil((d.bottom() + 1) / s)) - 1))
                        .intersected(QRect(0, 0, m_cubeRaw.cols(), m_cubeRaw.rows()));
    if (!r.isEmpty())
        handleSelection(r, false);
}

void HsiWorkspace::handleSelection(const QRect& r, bool isPoint)
{
    if (m_cube.isNull())
        return;
    const auto areaText = [&r] {
        return QCoreApplication::translate("iat::HsiWorkspace", "rows %1–%2, columns %3–%4")
            .arg(r.top() + 1).arg(r.bottom() + 1).arg(r.left() + 1).arg(r.right() + 1);
    };
    switch (m_tool) {
    case Tool::Pan:
        break;
    case Tool::Spectrum:
        addRoi(r, isPoint ? tr("Point %1").arg(m_roiCounter + 1) : tr("Area %1").arg(m_roiCounter + 1));
        break;
    case Tool::WhiteReference:
        m_whiteRef = r;
        updateWhiteLabel();
        m_reflectance->setChecked(true);  // starts the processing
        scheduleUpdate(false);
        setTool(m_previousTool);
        break;
    case Tool::ColourReference:
        m_colourRef = r;
        m_wb->setCurrentIndex(int(WhiteBalance::Reference));
        invalidateColour();
        setTool(m_previousTool);
        setViewMode(ViewMode::Colour);
        renderView();
        break;
    case Tool::SimilarityReference: {
        const RegionStats st = regionStats(m_cube, toProcessed(r));
        if (st.isValid()) {
            m_similarityArea = isPoint ? QRect() : r;
            setSimilarityReference(st.mean, isPoint ? tr("pixel (%1, %2)").arg(r.top() + 1).arg(r.left() + 1) : areaText());
        }
        setTool(m_previousTool);
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// Analysis
// ---------------------------------------------------------------------------

void HsiWorkspace::addRoi(const QRect& rect, const QString& name)
{
    const QRect r = rect.intersected(QRect(0, 0, m_cubeRaw.cols(), m_cubeRaw.rows()));
    if (r.isEmpty())
        return;
    Roi roi;
    roi.rect = r;
    roi.color = roiColour(m_roiCounter);
    roi.name = name.isEmpty() ? tr("Area %1").arg(m_roiCounter + 1) : name;
    ++m_roiCounter;
    m_rois << roi;
    updateSpectra();
    if (anyPaneShows(ViewMode::Matches))
        renderView();
    else
        updateOverlays();
}

void HsiWorkspace::clearRois()
{
    m_rois.clear();
    m_keptSelections.clear();
    m_roiCounter = 0;
    updateSpectra();
    if (anyPaneShows(ViewMode::Matches))
        renderView();
    else
        updateOverlays();
}

void HsiWorkspace::setSimilarityReference(const QVector<double>& spectrum, const QString& name)
{
    m_similarityRef = spectrum;
    m_similarityName = name;
    if (viewMode() != ViewMode::Similarity)
        setViewMode(ViewMode::Similarity);
    else
        renderView();
    updateSpectra();
}

void HsiWorkspace::setMatchAngle(double degrees)
{
    m_matchAngle->setValue(degrees);
}

void HsiWorkspace::setSimilarityThreshold(double degrees)
{
    m_simThreshold->setValue(degrees);
}

qint64 HsiWorkspace::similarPixelCount() const
{
    return std::count(m_similarMask.begin(), m_similarMask.end(), quint8(1));
}

void HsiWorkspace::runClustering(int k)
{
    if (m_cube.isNull() || m_clusterWatcher->isRunning())
        return;
    {
        const QSignalBlocker block(m_clusterK);
        m_clusterK->setValue(k);
    }
    const Cube cube = m_cube;
    const bool shape = m_clusterShape->isChecked();
    m_clusterLabel->setText(tr("Finding %1 classes …").arg(k));
    m_clusterWatcher->setFuture(QtConcurrent::run([cube, k, shape] { return kMeans(cube, k, 30, shape); }));
    updateBusy();
    if (viewMode() != ViewMode::Classes)
        setViewMode(ViewMode::Classes);
}

void HsiWorkspace::updateSpectra()
{
    QList<SpectrumPlot::Series> series;
    {
        const QSignalBlocker block(m_roiList);
        const int current = m_roiList->currentRow();
        m_roiList->clear();
        for (const Roi& r : std::as_const(m_rois)) {
            const RegionStats st = regionStats(m_cube, toProcessed(r.rect));
            QPixmap swatch(12, 12);
            swatch.fill(r.color);
            auto* item = new QListWidgetItem(QIcon(swatch), r.rect.width() * r.rect.height() == 1
                                                                ? tr("%1 · row %2, column %3").arg(r.name).arg(r.rect.top() + 1).arg(r.rect.left() + 1)
                                                                : tr("%1 · %2 × %3 px").arg(r.name).arg(r.rect.height()).arg(r.rect.width()));
            m_roiList->addItem(item);
            if (st.isValid())
                series << SpectrumPlot::Series{r.name, r.color, st.mean, st.count > 1 ? st.std : QVector<double>(), false};
        }
        for (const SpectrumPlot::Series& kept : std::as_const(m_keptSelections)) {
            QPixmap swatch(12, 12);
            swatch.fill(kept.color);
            m_roiList->addItem(new QListWidgetItem(QIcon(swatch), kept.name));
        }
        if (current >= 0 && current < m_roiList->count())
            m_roiList->setCurrentRow(current);
    }
    if (viewMode() == ViewMode::Similarity && m_similarityRef.size() == m_cube.bands()) {
        series << SpectrumPlot::Series{tr("Reference"), QColor(0, 200, 235), m_similarityRef, {}, true};
        const RegionStats st = maskStats(m_cube, m_similarMask);
        if (st.isValid())
            series << SpectrumPlot::Series{tr("Similar area (%1 px)").arg(st.count), QColor(0, 150, 200), st.mean, {}, false};
    }
    if (viewMode() == ViewMode::Classes && m_plotClasses->isChecked() && m_clustering.isValid()
        && m_clustering.labels.size() == m_cube.planeSize())
        for (int i = 0; i < m_clustering.k; ++i)
            series << SpectrumPlot::Series{tr("Class %1").arg(i + 1), classColor(i), m_clustering.centres[i], {}, false};
    for (const SpectrumPlot::Series& kept : std::as_const(m_keptSelections))
        if (kept.values.size() == m_cube.bands())
            series << kept;
    if (m_selectionMask.size() == m_cube.planeSize()) {
        const RegionStats st = maskStats(m_cube, m_selectionMask);
        if (st.isValid())
            series << SpectrumPlot::Series{tr("Selection (%1 px)").arg(st.count), kSelectionColour, st.mean,
                                           st.count > 1 ? st.std : QVector<double>(), false};
    }
    if (!m_hoverSpectrum.isEmpty())
        series << SpectrumPlot::Series{m_hoverName, palette().color(QPalette::Text), m_hoverSpectrum, {}, true};
    m_plot->setSeries(series);
    updateScoreGroups();
}

// ---------------------------------------------------------------------------
// Score plot (PCA) and pixel selection
// ---------------------------------------------------------------------------

QString HsiWorkspace::unitsLabel(const QString& units) const
{
    if (units == QLatin1String("reflectance"))
        return tr("Reflectance");
    if (units == QLatin1String("absorbance"))
        return tr("Absorbance");
    if (units == QLatin1String("normalised"))
        return tr("Normalised value");
    if (units == QLatin1String("derivative"))
        return tr("Derivative");
    return tr("Value (DN)");
}

void HsiWorkspace::updateScores()
{
    if (m_cube.isNull() || m_cube.bands() < 2) {
        m_scoreXImage = m_scoreYImage = Image2D();
        m_scorePlot->clearScores();
        m_loadingsPlot->setSeries({});
        return;
    }
    if (m_rightTabs->currentIndex() != 1)
        return;  // computed when the tab is shown
    if (!m_pca.isValid())
        m_pca = principalComponents(m_cube);
    if (!m_pca.isValid())
        return;
    const int n = m_pca.bands;
    if (m_scoreX->count() != n) {
        const QSignalBlocker bx(m_scoreX), by(m_scoreY);
        m_scoreX->clear();
        m_scoreY->clear();
        for (int i = 0; i < n; ++i) {
            m_scoreX->addItem(tr("PC%1").arg(i + 1));
            m_scoreY->addItem(tr("PC%1").arg(i + 1));
        }
        m_scoreX->setCurrentIndex(0);
        m_scoreY->setCurrentIndex(1);
        m_scoreXIndex = m_scoreYIndex = -1;
    }
    const QVector<double> e = m_pca.explained();
    for (int i = 0; i < n; ++i) {
        const QString text = tr("PC%1 (%2 %)").arg(i + 1).arg(100.0 * e.value(i), 0, 'f', 1);
        m_scoreX->setItemText(i, text);
        m_scoreY->setItemText(i, text);
    }
    const int ix = m_scoreX->currentIndex(), iy = m_scoreY->currentIndex();
    if (ix != m_scoreXIndex || iy != m_scoreYIndex || m_scoreXImage.isNull()) {
        m_scoreXIndex = ix;
        m_scoreYIndex = iy;
        m_scoreXImage = pcaComponent(m_cube, m_pca, ix);
        m_scoreYImage = pcaComponent(m_cube, m_pca, iy);
        m_scorePlot->setScores(m_scoreXImage, m_scoreYImage, m_scoreX->currentText(), m_scoreY->currentText());
        QList<SpectrumPlot::Series> loadings;
        for (int k : {ix, iy}) {
            QVector<double> v(n);
            for (int b = 0; b < n; ++b)
                v[b] = m_pca.vectors[b * n + k];
            loadings << SpectrumPlot::Series{tr("PC%1").arg(k + 1), k == ix ? QColor(31, 119, 180) : QColor(214, 39, 40), v, {}, false};
            if (ix == iy)
                break;
        }
        m_loadingsPlot->setWavelengths(m_cube.hasWavelengths() ? m_cube.wavelengths : QVector<double>());
        m_loadingsPlot->setSeries(loadings);
    }
    updateScoreGroups();
}

void HsiWorkspace::updateScoreGroups()
{
    if (m_scoreXImage.isNull() || m_scoreXImage.width() != m_cube.cols() || m_scoreXImage.height() != m_cube.rows())
        return;
    QList<ScorePlot::Group> groups;
    for (const Roi& r : std::as_const(m_rois)) {
        const QRect rect = toProcessed(r.rect).intersected(QRect(0, 0, m_cube.cols(), m_cube.rows()));
        ScorePlot::Group g{r.color, {}};
        for (int y = rect.top(); y <= rect.bottom(); ++y)
            for (int x = rect.left(); x <= rect.right(); ++x)
                g.points << QPointF(m_scoreXImage.at(x, y), m_scoreYImage.at(x, y));
        groups << g;
    }
    m_scorePlot->setGroups(groups);
}

void HsiWorkspace::selectScores(const QPolygonF& polygon)
{
    if (m_scoreXImage.isNull() || m_scoreXImage.width() != m_cube.cols() || m_scoreXImage.height() != m_cube.rows()) {
        showScores();
        if (m_scoreXImage.isNull())
            return;
    }
    m_selectionMask = selectInPolygon(m_scoreXImage, m_scoreYImage, polygon);
    m_scorePlot->setSelection(polygon);
    updateSelectionInfo();
    renderView();
    updateSpectra();
}

void HsiWorkspace::clearScoreSelection()
{
    m_selectionMask.clear();
    m_scorePlot->setSelection(QPolygonF());
    updateSelectionInfo();
    renderView();
    updateSpectra();
}

void HsiWorkspace::keepScoreSelection()
{
    if (m_selectionMask.size() != m_cube.planeSize())
        return;
    const RegionStats st = maskStats(m_cube, m_selectionMask);
    if (!st.isValid())
        return;
    // Kept as a fixed spectrum: the pixels need not form a rectangle.
    m_keptSelections << SpectrumPlot::Series{tr("Selection %1 (%2 px)").arg(m_keptSelections.size() + 1).arg(st.count),
                                             classColor(int(m_keptSelections.size()) + 3), st.mean, {}, false};
    updateSpectra();
}

void HsiWorkspace::removeKeptSelection(int index)
{
    if (index < 0 || index >= m_keptSelections.size())
        return;
    m_keptSelections.removeAt(index);
    updateSpectra();
}

qsizetype HsiWorkspace::selectionCount() const
{
    return std::count(m_selectionMask.begin(), m_selectionMask.end(), quint8(1));
}

void HsiWorkspace::showScores()
{
    m_rightTabs->setCurrentIndex(1);
    updateScores();
}

void HsiWorkspace::setScoreComponents(int x, int y)
{
    showScores();
    m_scoreX->setCurrentIndex(x);
    m_scoreY->setCurrentIndex(y);
}

void HsiWorkspace::updateSelectionInfo()
{
    const qsizetype n = m_selectionMask.size() == m_cube.planeSize() ? selectionCount() : 0;
    m_scoreInfo->setText(n > 0 ? tr("%1 pixels selected (%2 %)").arg(n).arg(100.0 * double(n) / double(m_cube.planeSize()), 0, 'f', 1)
                               : tr("No selection."));
    m_clearSelection->setEnabled(n > 0);
    m_keepSelection->setEnabled(n > 0);
}

void HsiWorkspace::tintSelection(QImage& img) const
{
    if (m_selectionMask.size() != m_cube.planeSize() || img.width() != m_cube.cols() || img.height() != m_cube.rows())
        return;
    img = img.convertToFormat(QImage::Format_RGB32);
    const int cols = m_cube.cols();
    for (int y = 0; y < img.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < cols; ++x)
            if (m_selectionMask[qsizetype(y) * cols + x]) {
                const QRgb c = line[x];
                line[x] = qRgb((qRed(c) + kSelectionColour.red()) / 2, (qGreen(c) + kSelectionColour.green()) / 2,
                               (qBlue(c) + kSelectionColour.blue()) / 2);
            }
    }
}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------

bool HsiWorkspace::exportCube(const QString& path, CubeFormat format, SampleType type, bool unprocessed, QString* error)
{
    const Cube& c = unprocessed ? m_cubeRaw : m_cube;
    QString err;
    bool ok = false;
    if (c.isNull())
        err = tr("There is no cube to save.");
    else if (format == CubeFormat::Envi)
        ok = writeEnvi(c, path, type, Interleave::Bsq, &err, m_cubeLayout);
    else if (format == CubeFormat::Mat)
        ok = writeMat(c, path, type, &err, m_cubeLayout);
    else
        ok = writeMultiPageTiff(c, path, type, &err);
    if (error)
        *error = err;
    report(ok ? tr("Saved %1").arg(QDir::toNativeSeparators(format == CubeFormat::Envi ? enviHeaderPath(path) : path)) : err);
    return ok;
}

bool HsiWorkspace::exportView(const QString& path)
{
    QImage img = m_panes[0].shown;
    if (m_paneCount > 1 && !img.isNull()) {
        // All views side by side (2) or in a 2 × 2 grid (4), each in a cell
        // the size of the first view.
        const QSize cell = img.size();
        const int cols = 2, rows = m_paneCount / 2, gap = 8;
        QImage out(cols * cell.width() + gap, rows * cell.height() + (rows - 1) * gap, QImage::Format_RGB888);
        out.fill(QColor(40, 40, 40));
        QPainter painter(&out);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        for (int i = 0; i < m_paneCount; ++i) {
            const QImage& pi = m_panes[size_t(i)].shown;
            if (pi.isNull())
                continue;
            const QImage scaled = pi.scaled(cell, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            const QPoint origin((i % cols) * (cell.width() + gap) + (cell.width() - scaled.width()) / 2,
                                (i / cols) * (cell.height() + gap) + (cell.height() - scaled.height()) / 2);
            painter.drawImage(origin, scaled);
        }
        painter.end();
        img = out;
    }
    const bool ok = !img.isNull() && img.save(path);
    report(ok ? tr("Saved %1").arg(QDir::toNativeSeparators(path)) : tr("Could not save %1").arg(QDir::toNativeSeparators(path)));
    return ok;
}

bool HsiWorkspace::exportSpectra(const QString& path, QString* error)
{
    QList<SpectrumSeries> out;
    for (const Roi& r : std::as_const(m_rois)) {
        const RegionStats st = regionStats(m_cube, toProcessed(r.rect));
        if (st.isValid())
            out << SpectrumSeries{QStringLiteral("%1 (rows %2-%3, cols %4-%5)").arg(r.name).arg(r.rect.top() + 1)
                                      .arg(r.rect.bottom() + 1).arg(r.rect.left() + 1).arg(r.rect.right() + 1),
                                  st.mean, st.std, st.count};
    }
    if (m_similarityRef.size() == m_cube.bands()) {
        const RegionStats st = maskStats(m_cube, m_similarMask);
        if (st.isValid())
            out << SpectrumSeries{QStringLiteral("similar area (within %1 deg)").arg(m_simThreshold->value()), st.mean, st.std, st.count};
    }
    if (m_clustering.isValid() && m_clustering.labels.size() == m_cube.planeSize())
        for (int i = 0; i < m_clustering.k; ++i)
            out << SpectrumSeries{QStringLiteral("class %1").arg(i + 1), m_clustering.centres[i], {}, m_clustering.sizes[i]};
    QString err;
    bool ok = false;
    if (out.isEmpty())
        err = tr("There are no spectra to save: keep a point or an area with the spectrum tool first.");
    else
        ok = writeSpectraCsv(path, m_cube.hasWavelengths() ? m_cube.wavelengths : QVector<double>(), out, &err);
    if (error)
        *error = err;
    report(ok ? tr("Saved %1").arg(QDir::toNativeSeparators(path)) : err);
    return ok;
}

void HsiWorkspace::exportCubeDialog()
{
    if (m_cube.isNull()) {
        report(tr("There is no cube to save."));
        return;
    }
    const CubeFormat format = CubeFormat(m_exportFormat->currentIndex());
    const QString ext = format == CubeFormat::Envi ? QStringLiteral(".hdr") : format == CubeFormat::Mat ? QStringLiteral(".mat")
                                                                                                         : QStringLiteral(".tif");
    const QString filter = format == CubeFormat::Envi ? tr("ENVI header (*.hdr)")
                           : format == CubeFormat::Mat ? tr("MAT-files (*.mat)")
                                                       : tr("TIFF files (*.tif *.tiff)");
    const QString path = QFileDialog::getSaveFileName(this, tr("Save cube"),
                                                      QDir(lastDir()).filePath(baseName() + QStringLiteral("_cube") + ext), filter);
    if (path.isEmpty())
        return;
    rememberDir(path);
    exportCube(path, format, SampleType(m_exportType->currentIndex()), m_exportUnprocessed->isChecked());
}

void HsiWorkspace::exportViewDialog()
{
    if (m_shown.isNull())
        return;
    static const char* suffixes[] = {"_mosaic", "_band", "_bands", "_sRGB", "_falsecolour", "_pca", "_index", "_similar", "_classes", "_matches"};
    QString name = baseName() + QLatin1String(suffixes[int(viewMode())]);
    if (viewMode() == ViewMode::Band)
        name += QString::number(band() + 1);
    const QString path = QFileDialog::getSaveFileName(this, tr("Save view"), QDir(lastDir()).filePath(name + QStringLiteral(".png")),
                                                      tr("PNG images (*.png)"));
    if (path.isEmpty())
        return;
    rememberDir(path);
    exportView(path);
}

void HsiWorkspace::exportSpectraDialog()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Save spectra"),
                                                      QDir(lastDir()).filePath(baseName() + QStringLiteral("_spectra.csv")),
                                                      tr("CSV files (*.csv)"));
    if (path.isEmpty())
        return;
    rememberDir(path);
    exportSpectra(path);
}

// ---------------------------------------------------------------------------
// Correction: spectral correction, reference frames, sensor profile
// ---------------------------------------------------------------------------

void HsiWorkspace::updateCorrection()
{
    SpectralCorrection c;
    if (m_corr.spectralCorrection) {
        if (!m_corr.matrixPath.isEmpty())
            c = m_corr.loadedMatrix;
        else if (!m_response.isNull())
            c = computeCorrection(m_response, m_corr.targetFwhm, m_corr.regularisation);
    }
    m_correction = c;
    m_xyz.clear();
    invalidateColour();
    const bool available = !m_response.isNull() || m_corr.loadedMatrix.isValid();
    m_correctionCheck->setEnabled(available);
    const QSignalBlocker block(m_correctionCheck);
    m_correctionCheck->setChecked(m_corr.spectralCorrection && available);
}

void HsiWorkspace::updateCorrectionStatus()
{
    QStringList lines;
    if (!m_response.isNull())
        lines << tr("%1 · %2 channels").arg(m_responseName).arg(m_response.bands());
    else if (m_directRgb.size() > 0)
        lines << tr("%1 · colour matrix only").arg(m_responseName);
    else
        lines << tr("No spectral response.");
    if (m_correction.isValid())
        lines << (m_corr.matrixPath.isEmpty() ? tr("Correction: %1 % residual, noise × %2")
                                                    .arg(100.0 * m_correction.residual, 0, 'f', 1).arg(noiseGain(m_correction), 0, 'f', 1)
                                              : tr("Correction: %1, noise × %2").arg(QFileInfo(m_corr.matrixPath).fileName())
                                                    .arg(noiseGain(m_correction), 0, 'f', 1));
    m_responseStatus->setText(lines.join(QLatin1Char('\n')));
}

void HsiWorkspace::scheduleCorrection()
{
    m_needCorrect = true;
    scheduleUpdate(false);
}

void HsiWorkspace::applyCorrectionSettings(const CorrectionSettings& s)
{
    const bool framesChanged = s.sceneExposure != m_corr.sceneExposure || s.whiteExposure != m_corr.whiteExposure
                               || s.whiteReflectance != m_corr.whiteReflectance;
    m_corr = s;
    m_frames.sceneExposure = s.sceneExposure;
    m_frames.whiteExposure = s.whiteExposure;
    m_frames.whiteReflectance = s.whiteReflectance;
    updateCorrection();
    updateCorrectionStatus();
    applyWavelengths();
    if (framesChanged && m_frames.any())
        scheduleUpdate(true);
    scheduleCorrection();
}

QString HsiWorkspace::responseDetails() const
{
    if (m_response.isNull())
        return m_directRgb.isEmpty() ? QString() : tr("%1\n%2").arg(m_responseName, m_responseNotes);
    QStringList peaks, widths;
    const QVector<double> w = m_response.fwhm();
    for (int b = 0; b < m_response.bands(); ++b) {
        peaks << QString::number(m_response.peaks()[b], 'f', 0);
        widths << QString::number(w[b], 'f', 0);
    }
    return tr("%1\n%2\nFilter peaks (nm): %3\nPeak widths, FWHM (nm): %4")
        .arg(QDir::toNativeSeparators(m_responsePath), m_responseNotes, peaks.join(QStringLiteral(", ")),
             widths.join(QStringLiteral(", ")));
}

QString HsiWorkspace::framesDetails() const
{
    QStringList lines;
    lines << tr("Dark: %1").arg(m_frames.dark.isNull() ? tr("none") : m_darkName);
    lines << tr("White: %1").arg(m_frames.white.isNull() ? tr("none") : m_whiteName);
    if (!m_fromMosaic && m_frames.any())
        lines << tr("The frames apply to raw mosaics, not to a cube opened from a file.");
    return lines.join(QLatin1Char('\n'));
}

void HsiWorkspace::setReferenceFrame(bool white, const Image2D& frame, const QString& name, const QString& path)
{
    (white ? m_frames.white : m_frames.dark) = frame;
    (white ? m_whiteName : m_darkName) = name;
    (white ? m_whitePath : m_darkPath) = path;
    QLabel* label = white ? m_whiteLabel2 : m_darkLabel;
    label->setText(frame.isNull() ? tr("none") : name);
    label->setToolTip(frame.isNull() ? QString() : QStringLiteral("%1 (%2 × %3)").arg(name).arg(frame.height()).arg(frame.width()));
    (white ? m_clearWhiteFrame : m_clearDark)->setEnabled(!frame.isNull());
    m_blackLevel->setEnabled(m_frames.dark.isNull());
    m_blackLevel->setToolTip(m_frames.dark.isNull()
                                 ? tr("Dark offset in DN, subtracted from every pixel. Leave 0 for a calibrated image "
                                      "(light − dark), which has it removed already.")
                                 : tr("Not used: the dark frame is subtracted instead."));
    if (!frame.isNull() && !m_mosaic.isNull() && !frame.sameSize(m_mosaic))
        report(tr("The %1 frame is %2 × %3, the image %4 × %5: they must be the same size.")
                   .arg(white ? tr("white") : tr("dark")).arg(frame.height()).arg(frame.width())
                   .arg(m_mosaic.height()).arg(m_mosaic.width()));
    if (!m_fromMosaic && !m_cubeExtracted.isNull() && !frame.isNull())
        report(tr("Reference frames apply to raw mosaics; this cube was opened from a file."));
    scheduleUpdate(true);
}

bool HsiWorkspace::loadReferenceFrame(bool white, const QString& path)
{
    rememberDir(path);
    ImageStack stack;
    QString error;
    if (QFileInfo(path).suffix().compare(QLatin1String("mat"), Qt::CaseInsensitive) == 0) {
        const MatFileContents c = MatFileReader::read(path);
        const QList<MatVariable> stacks = c.ok() ? c.imageStacks() : QList<MatVariable>();
        if (stacks.isEmpty())
            error = c.ok() ? tr("%1 holds no image.").arg(QFileInfo(path).fileName()) : c.error;
        else
            stack = stacks.first().data;
    } else {
        const ImageFrame f = ImageFileReader::read(path);
        if (f.ok())
            stack = f.data;
        else
            error = f.error;
    }
    if (!error.isEmpty()) {
        report(error);
        return false;
    }
    const Image2D mean = processing::meanOverFrames(stack);
    QString name = QFileInfo(path).fileName();
    if (stack.frameCount() > 1)
        name += tr(" (mean of %1 frames)").arg(stack.frameCount());
    setReferenceFrame(white, mean, name, QFileInfo(path).absoluteFilePath());
    return true;
}

void HsiWorkspace::importReferenceFrame(bool white)
{
    const QList<ImportCandidate> candidates = m_importProvider ? m_importProvider() : QList<ImportCandidate>();
    ImportImageDialog dialog(candidates, this);
    dialog.setWindowTitle(white ? tr("White reference frame from workspace") : tr("Dark frame from workspace"));
    if (dialog.exec() != QDialog::Accepted || dialog.selectedIndex() < 0)
        return;
    const ImportCandidate& c = candidates[dialog.selectedIndex()];
    setReferenceFrame(white, c.image, QStringLiteral("%1 · %2").arg(c.tabName, c.imageName));
}

void HsiWorkspace::clearReferenceFrame(bool white)
{
    setReferenceFrame(white, Image2D(), QString());
}

void HsiWorkspace::openCorrectionSettings()
{
    CorrectionDialog dialog(m_response, responseDetails(), m_corr, framesDetails(), this);
    dialog.onSaveProfile = [this](const CorrectionSettings& s) {
        applyCorrectionSettings(s);
        const QString path = QFileDialog::getSaveFileName(this, tr("Save sensor profile"),
                                                          QDir(lastDir()).filePath(QStringLiteral("sensor.iatsensor")),
                                                          tr("Sensor profiles (*.iatsensor)"));
        if (!path.isEmpty())
            saveSensorProfile(path);
    };
    dialog.onLoadProfile = [this] {
        const QString path = QFileDialog::getOpenFileName(this, tr("Load sensor profile"), lastDir(),
                                                          tr("Sensor profiles (*.iatsensor);;All files (*)"));
        return !path.isEmpty() && loadSensorProfile(path);
    };
    if (dialog.exec() == QDialog::Accepted)
        applyCorrectionSettings(dialog.settings());
}

bool HsiWorkspace::saveSensorProfile(const QString& path)
{
    rememberDir(path);
    SensorProfile p;
    const ExtractOptions o = extractOptions();
    p.name = QFileInfo(path).completeBaseName();
    p.layout = o.layout;
    p.bitDepth = o.bitDepth;
    p.blackLevel = o.blackLevel;
    p.clipNegative = o.clipNegative;
    p.method = int(o.method);
    p.responsePath = m_responsePath;
    p.spectralCorrection = m_corr.spectralCorrection;
    p.correctionFwhm = m_corr.targetFwhm;
    p.correctionRegularisation = m_corr.regularisation;
    p.correctionMatrixPath = m_corr.matrixPath;
    p.darkPath = m_darkPath;
    p.whitePath = m_whitePath;
    p.sceneExposure = m_corr.sceneExposure;
    p.whiteExposure = m_corr.whiteExposure;
    p.whiteReflectance = m_corr.whiteReflectance;
    QString error;
    const bool ok = writeSensorProfile(p, path, &error);
    report(ok ? tr("Saved sensor profile %1").arg(QDir::toNativeSeparators(path)) : error);
    return ok;
}

bool HsiWorkspace::loadSensorProfile(const QString& path)
{
    QString error;
    const auto p = readSensorProfile(path, &error);
    if (!p) {
        report(error);
        return false;
    }
    rememberDir(path);
    QStringList missing;
    ExtractOptions o = extractOptions();
    o.layout = p->layout;
    o.bitDepth = p->bitDepth;
    o.blackLevel = p->blackLevel;
    o.clipNegative = p->clipNegative;
    o.method = ExtractMethod(std::clamp(p->method, 0, 3));
    setExtractOptions(o);
    if (!p->responsePath.isEmpty()) {
        if (QFileInfo::exists(p->responsePath))
            loadResponse(p->responsePath);
        else
            missing << p->responsePath;
    }
    CorrectionSettings s = m_corr;
    s.spectralCorrection = p->spectralCorrection;
    s.targetFwhm = p->correctionFwhm;
    s.regularisation = p->correctionRegularisation;
    s.matrixPath.clear();
    s.loadedMatrix = SpectralCorrection();
    if (!p->correctionMatrixPath.isEmpty()) {
        const CorrectionFile f = loadCorrectionMatrix(p->correctionMatrixPath, m_response.isNull() ? kBands : m_response.bands());
        if (f.ok()) {
            s.matrixPath = p->correctionMatrixPath;
            s.loadedMatrix = f.correction;
        } else {
            missing << p->correctionMatrixPath;
        }
    }
    s.sceneExposure = p->sceneExposure;
    s.whiteExposure = p->whiteExposure;
    s.whiteReflectance = p->whiteReflectance;
    applyCorrectionSettings(s);
    for (bool white : {false, true}) {
        const QString& fp = white ? p->whitePath : p->darkPath;
        if (fp.isEmpty())
            clearReferenceFrame(white);
        else if (!QFileInfo::exists(fp) || !loadReferenceFrame(white, fp))
            missing << fp;
    }
    report(missing.isEmpty() ? tr("Sensor profile loaded: %1").arg(QFileInfo(path).fileName())
                             : tr("Sensor profile loaded; not found: %1").arg(missing.join(QStringLiteral(", "))));
    return true;
}


// ---------------------------------------------------------------------------
// Drag and drop
// ---------------------------------------------------------------------------

void HsiWorkspace::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
        for (const QUrl& url : event->mimeData()->urls())
            if (url.isLocalFile()) {
                event->acceptProposedAction();
                return;
            }
}

void HsiWorkspace::dropEvent(QDropEvent* event)
{
    for (const QUrl& url : event->mimeData()->urls())
        if (url.isLocalFile())
            loadFile(url.toLocalFile());
    event->acceptProposedAction();
}

} // namespace iat
