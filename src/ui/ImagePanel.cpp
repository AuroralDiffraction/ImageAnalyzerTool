#include "ui/ImagePanel.h"

#include "core/Processing.h"
#include "io/ImageFileReader.h"
#include "ui/ContrastControl.h"
#include "ui/HistogramWidget.h"
#include "ui/ImageView.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFontInfo>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <tuple>

namespace iat {

namespace {

// Automatic contrast: clip the darkest and brightest 0.5 % of pixels.
constexpr double kAutoLow = 0.005;
constexpr double kAutoHigh = 0.995;

QString formatValue(double v) { return QString::number(v, 'g', 7); }

QColor accentFor(PanelRole role)
{
    switch (role) {
    case PanelRole::Light:      return QColor(0xE0, 0xA0, 0x30);
    case PanelRole::Dark:       return QColor(0x4A, 0x6F, 0xA5);
    case PanelRole::Calibrated: return QColor(0x2E, 0x9E, 0x8F);
    }
    return Qt::gray;
}

QString titleFor(PanelRole role)
{
    switch (role) {
    case PanelRole::Light:      return QObject::tr("Light frames");
    case PanelRole::Dark:       return QObject::tr("Dark frames");
    case PanelRole::Calibrated: return QObject::tr("Calibrated frames");
    }
    return {};
}

QString placeholderFor(PanelRole role)
{
    switch (role) {
    case PanelRole::Light:
        return QObject::tr("Drop light frames here:\n"
                           "a .mat file with an X × Y × Z array (Z frames),\n"
                           "or one or more TIFF, PNG or JPG images (one frame each)\n\n"
                           "or click Load…");
    case PanelRole::Dark:
        return QObject::tr("Drop dark frames here:\n"
                           "a .mat file with an X × Y × Z array (Z frames),\n"
                           "or one or more TIFF, PNG or JPG images (one frame each)\n\n"
                           "In Mean mode, the pixel-by-pixel mean over all frames\n"
                           "becomes the dark frame.\n\n"
                           "or click Load…");
    case PanelRole::Calibrated:
        return QObject::tr("Load light frames and dark frames.\n\n"
                           "This panel then shows\n"
                           "light image − dark image\n"
                           "(the two images on display).\n\n"
                           "To show a calibrated image from a file instead,\n"
                           "drop a .mat, TIFF, PNG or JPG file here.");
    }
    return {};
}

// Small red disc for the "Auto calibration off" indicator.
QIcon redDotIcon()
{
    QIcon icon;
    for (int size : {10, 20, 30}) {
        QPixmap pm(size, size);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xD3, 0x2F, 0x2F));
        p.drawEllipse(QRectF(0.5, 0.5, size - 1.0, size - 1.0));
        p.end();
        icon.addPixmap(pm);
    }
    return icon;
}

void mute(QLabel* label)
{
    QPalette p = label->palette();
    p.setColor(QPalette::WindowText, p.color(QPalette::PlaceholderText));
    label->setPalette(p);
}

QLabel* sectionTitle(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text, parent);
    QFont f = l->font();
    f.setBold(true);
    l->setFont(f);
    return l;
}

} // namespace

ImagePanel::ImagePanel(PanelRole role, QWidget* parent)
    : QFrame(parent)
    , m_role(role)
    , m_defaultPlaceholder(placeholderFor(role))
{
    setObjectName(QStringLiteral("imagePanel"));
    setStyleSheet(QStringLiteral(
        "QFrame#imagePanel { border: 1px solid palette(mid); border-radius: 6px; }"
        "QFrame#imagePanel[dropActive=\"true\"] { border: 2px solid palette(highlight); }"
        "QLabel#dropZone { border: 2px dashed palette(mid); border-radius: 8px; padding: 16px; }"));
    setAcceptDrops(true);
    setMinimumWidth(300);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);

    // Accent strip + title row.
    auto* accent = new QFrame(this);
    accent->setFixedHeight(4);
    accent->setStyleSheet(QStringLiteral("background: %1; border-radius: 2px;").arg(accentFor(role).name()));
    layout->addWidget(accent);

    // Title row: Title [● Auto calibration off]  ...  [Single frame|Mean] [Load…]
    auto* header = new QHBoxLayout;
    m_title = new QLabel(titleFor(role), this);
    QFont tf = m_title->font();
    tf.setBold(true);
    tf.setPointSizeF(tf.pointSizeF() + 2);
    m_title->setFont(tf);
    header->addWidget(m_title);
    if (role == PanelRole::Calibrated) {
        m_autoOffButton = new QToolButton(this);
        m_autoOffButton->setObjectName(QStringLiteral("autoCalibrationOff"));
        m_autoOffButton->setText(tr("Auto calibration off"));
        m_autoOffButton->setIcon(redDotIcon());
        m_autoOffButton->setIconSize(QSize(10, 10));
        m_autoOffButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_autoOffButton->setAutoRaise(true);
        m_autoOffButton->setCursor(Qt::PointingHandCursor);
        m_autoOffButton->setStyleSheet(QStringLiteral("QToolButton { color: #C62828; font-weight: bold; }"));
        m_autoOffButton->setToolTip(tr("This panel shows an image loaded from a file, so it is not calculated from\n"
                                       "the light and dark frames.\n\n"
                                       "Click to remove the loaded image and turn automatic calibration\n"
                                       "(light image − dark image) back on."));
        connect(m_autoOffButton, &QToolButton::clicked, this, &ImagePanel::autoCalibrationRequested);
        m_autoOffButton->setVisible(false);
        header->addWidget(m_autoOffButton);
    }
    header->addStretch(1);
    if (role != PanelRole::Calibrated) {
        // Single frame / Mean toggle: two buttons that work as one switch.
        auto makeModeButton = [this](const QString& text, const QString& tip) {
            auto* b = new QToolButton(this);
            b->setText(text);
            b->setToolTip(tip);
            b->setCheckable(true);
            b->setToolButtonStyle(Qt::ToolButtonTextOnly);
            return b;
        };
        const QString stack = role == PanelRole::Light ? QStringLiteral("light") : QStringLiteral("dark");
        m_singleButton = makeModeButton(tr("Single frame"),
                                        tr("Show one frame of the stack: %1(:, :, k).\nChoose the frame below the image.")
                                            .arg(stack));
        m_meanButton = makeModeButton(tr("Mean"),
                                      tr("Show the pixel-by-pixel mean over all frames: mean(%1, 3)").arg(stack));
        m_modeGroup = new QButtonGroup(this);
        m_modeGroup->setExclusive(true);
        m_modeGroup->addButton(m_singleButton);
        m_modeGroup->addButton(m_meanButton);
        m_mode = role == PanelRole::Dark ? FrameMode::Mean : FrameMode::Single;
        m_singleButton->setChecked(m_mode == FrameMode::Single);
        m_meanButton->setChecked(m_mode == FrameMode::Mean);
        connect(m_singleButton, &QToolButton::toggled, this, [this](bool on) {
            if (on)
                setFrameMode(FrameMode::Single);
        });
        connect(m_meanButton, &QToolButton::toggled, this, [this](bool on) {
            if (on)
                setFrameMode(FrameMode::Mean);
        });
        auto* modeBox = new QHBoxLayout;
        modeBox->setSpacing(0);
        modeBox->addWidget(m_singleButton);
        modeBox->addWidget(m_meanButton);
        header->addLayout(modeBox);
        header->addSpacing(6);
    }
    m_loadButton = new QToolButton(this);
    m_loadButton->setText(tr("Load…"));
    m_loadButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_loadButton->setToolTip(
        role == PanelRole::Calibrated
            ? tr("Open a calibrated image: a .mat, TIFF, PNG or JPG file (or drop one onto this panel).\n"
                 "Automatic calibration then turns off.")
            : tr("Open a .mat file, or one or more TIFF, PNG or JPG images\n(or drop them onto this panel)"));
    connect(m_loadButton, &QToolButton::clicked, this, &ImagePanel::loadRequested);
    header->addWidget(m_loadButton);
    layout->addLayout(header);

    m_source = new QLabel(this);
    m_source->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_source->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mute(m_source);
    m_source->setText(role == PanelRole::Calibrated ? tr("Light image − dark image") : tr("No file loaded"));
    layout->addWidget(m_source);

    // Image area: placeholder or view.
    m_stack = new QStackedWidget(this);
    m_placeholder = new QLabel(m_defaultPlaceholder, this);
    m_placeholder->setObjectName(QStringLiteral("dropZone"));
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setWordWrap(true);
    mute(m_placeholder);
    m_view = new ImageView(this);
    m_stack->addWidget(m_placeholder);
    m_stack->addWidget(m_view);
    m_stack->setMinimumHeight(200);
    layout->addWidget(m_stack, 1);

    // Frame selector: Frame [◀] ----slider---- [▶] [ n ] of N
    m_frameBar = new QWidget(this);
    auto* fb = new QHBoxLayout(m_frameBar);
    fb->setContentsMargins(0, 0, 0, 0);
    m_frameLabel = new QLabel(tr("Frame"), m_frameBar);
    fb->addWidget(m_frameLabel);
    auto makeArrow = [this](Qt::ArrowType arrow, const QString& tip, int delta) {
        auto* b = new QToolButton(m_frameBar);
        b->setArrowType(arrow);
        b->setAutoRepeat(true);
        b->setAutoRepeatDelay(400);
        b->setAutoRepeatInterval(120);
        b->setToolTip(tip);
        connect(b, &QToolButton::clicked, this, [this, delta] { stepFrame(delta); });
        return b;
    };
    m_prevButton = makeArrow(Qt::LeftArrow, tr("Previous frame"), -1);
    m_nextButton = makeArrow(Qt::RightArrow, tr("Next frame"), +1);
    m_frameSlider = new QSlider(Qt::Horizontal, m_frameBar);
    m_frameSlider->setRange(1, 1);
    m_frameSpin = new QSpinBox(m_frameBar);
    m_frameSpin->setRange(1, 1);
    m_frameSpin->setToolTip(tr("Frame number (1-based, as in MATLAB)"));
    m_frameTotal = new QLabel(m_frameBar);
    fb->addWidget(m_prevButton);
    fb->addWidget(m_frameSlider, 1);
    fb->addWidget(m_nextButton);
    fb->addWidget(m_frameSpin);
    fb->addWidget(m_frameTotal);
    {
        // Every panel keeps this row's space, so the images in the three
        // panels have the same size and line up for comparison.
        QSizePolicy sp = m_frameBar->sizePolicy();
        sp.setRetainSizeWhenHidden(true);
        m_frameBar->setSizePolicy(sp);
    }
    m_frameBar->setVisible(false);
    layout->addWidget(m_frameBar);

    connect(m_frameSlider, &QSlider::valueChanged, this, [this](int v) {
        const QSignalBlocker block(m_frameSpin);
        m_frameSpin->setValue(v);
        updateFrameControls();
        emit currentFrameChanged(v - 1);
    });
    connect(m_frameSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        const QSignalBlocker block(m_frameSlider);
        m_frameSlider->setValue(v);
        updateFrameControls();
        emit currentFrameChanged(v - 1);
    });

    // Histogram header.
    auto* hh = new QHBoxLayout;
    hh->addWidget(sectionTitle(tr("Histogram"), this));
    hh->addStretch(1);
    m_logCheck = new QCheckBox(tr("Log"), this);
    m_logCheck->setToolTip(tr("Logarithmic pixel-count axis"));
    m_autoButton = new QToolButton(this);
    m_autoButton->setText(tr("Auto"));
    m_autoButton->setToolTip(tr("Stretch the contrast to the 0.5–99.5 % percentiles"));
    m_fullButton = new QToolButton(this);
    m_fullButton->setText(tr("Full range"));
    m_fullButton->setToolTip(tr("Show the full data range, from min to max"));
    hh->addWidget(m_logCheck);
    hh->addWidget(m_autoButton);
    hh->addWidget(m_fullButton);
    layout->addLayout(hh);

    m_histogram = new HistogramWidget(this);
    layout->addWidget(m_histogram);

    m_contrast = new ContrastControl(this);
    layout->addWidget(m_contrast);

    // Footer: pixel read-out and the statistics.
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    m_pixelLabel = new QLabel(this);
    m_pixelLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_pixelLabel->setToolTip(tr("Row and column are 1-based, as in MATLAB"));
    mute(m_pixelLabel);
    layout->addWidget(m_pixelLabel);

    // Mean / Std (calculateImageStats) and Noise (calculateImageNoise).
    auto* stats = new QGridLayout;
    stats->setContentsMargins(0, 2, 0, 0);
    stats->setHorizontalSpacing(20);
    stats->setVerticalSpacing(0);
    auto addStat = [&](int column, const QString& caption, QLabel*& captionLabel, QLabel*& value) {
        auto* c = new QLabel(caption, this);
        captionLabel = c;
        QFont cf = c->font();
        cf.setPointSizeF(std::max(7.0, cf.pointSizeF() - 1.0));
        c->setFont(cf);
        mute(c);
        value = new QLabel(QStringLiteral("–"), this);
        QFont vf = value->font();
        vf.setBold(true);
        vf.setPointSizeF(vf.pointSizeF() + 1);
        value->setFont(vf);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        stats->addWidget(c, 0, column);
        stats->addWidget(value, 1, column);
    };
    QLabel* noiseCaption = nullptr;
    addStat(0, tr("Mean [DN]"), m_meanCaption, m_meanValue);
    addStat(1, tr("Std [DN]"), m_stdCaption, m_stdValue);
    addStat(2, tr("Noise [DN]"), noiseCaption, m_noiseValue);
    stats->setColumnStretch(3, 1);
    layout->addLayout(stats);

    m_rangeLabel = new QLabel(this);
    m_rangeLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_rangeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mute(m_rangeLabel);
    layout->addWidget(m_rangeLabel);

    // Export section.
    auto* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line2);

    auto* ex = new QHBoxLayout;
    ex->addWidget(sectionTitle(tr("Export"), this));
    ex->addStretch(1);
    m_saveMatButton = new QToolButton(this);
    m_saveMatButton->setText(tr("Data (.mat)"));
    m_saveMatButton->setToolTip(tr("Save raw values to a .mat file: the image on display (double) with Mean, Std and\n"
                                   "Noise, or all frames of the stack behind this panel"));
    {
        auto* menu = new QMenu(m_saveMatButton);
        menu->addAction(tr("Image on display…"), this, [this] { emit exportRequested(ExportKind::FrameMat); });
        menu->addAction(tr("All frames…"), this, [this] { emit exportRequested(ExportKind::StackMat); });
        m_saveMatButton->setMenu(menu);
        m_saveMatButton->setPopupMode(QToolButton::InstantPopup);
    }
    m_savePngButton = new QToolButton(this);
    m_savePngButton->setText(tr("Image (.png)"));
    m_savePngButton->setToolTip(tr("Save the image as displayed (current contrast, full resolution) to a PNG file"));
    connect(m_savePngButton, &QToolButton::clicked, this, [this] { emit exportRequested(ExportKind::ImagePng); });
    ex->addWidget(m_saveMatButton);
    ex->addWidget(m_savePngButton);
    layout->addLayout(ex);

    // Wiring.
    connect(m_contrast, &ContrastControl::displayRangeChanged, this, &ImagePanel::setDisplayRange);
    connect(m_histogram, &HistogramWidget::displayRangeEdited, this, [this](double low, double high) {
        m_contrast->applyRange(low, high, low != m_low);
    });
    connect(m_logCheck, &QCheckBox::toggled, m_histogram, &HistogramWidget::setLogScale);
    connect(m_autoButton, &QToolButton::clicked, this, &ImagePanel::autoContrast);
    connect(m_fullButton, &QToolButton::clicked, this, &ImagePanel::fullRangeContrast);
    connect(m_view, &ImageView::hoveredPixel, this, &ImagePanel::showPixel);
    connect(m_view, &ImageView::contextMenuRequested, this, &ImagePanel::showContextMenu);

    updateStatTooltips();
    updateFrameControls();
    showPlaceholder(m_defaultPlaceholder);
}

QString ImagePanel::title() const { return m_title->text(); }

QSize ImagePanel::sizeHint() const
{
    QSize s = QFrame::sizeHint();
    s.setWidth(std::max(520, minimumSizeHint().width()));
    return s;
}
QString ImagePanel::sourceInfo() const { return m_source->text(); }

void ImagePanel::setImage(const Image2D& image, bool resetContrast)
{
    if (image.isNull()) {
        showPlaceholder(m_defaultPlaceholder);
        return;
    }

    m_image = image;
    m_stats = image.computeStats();
    m_meanStd = processing::calculateImageStats(image);
    if (!m_stats.isValid()) {
        const QString msg = tr("The image has no finite values (all NaN or Inf).");
        showPlaceholder(msg);
        return;
    }
    m_hist = computeHistogram(image, m_stats);

    m_contrast->setDataRange(m_stats.min, m_stats.max, m_stats.integerValued);

    double low = m_low;
    double high = m_high;
    if (resetContrast || !m_rangeValid) {
        // Default: automatic stretch, so a few hot or saturated pixels do not
        // squash the rest of the image into a narrow band of greys.
        std::tie(low, high) = processing::percentileRange(image, kAutoLow, kAutoHigh);
        if (!(std::isfinite(low) && std::isfinite(high) && high > low)) {
            low = m_stats.min;
            high = m_stats.max;
        }
    } else {
        low = std::clamp(low, m_stats.min, m_stats.max);
        high = std::clamp(high, m_stats.min, m_stats.max);
        if (!(high > low)) {
            low = m_stats.min;
            high = m_stats.max;
        }
    }
    if (!(high > low)) {  // constant image
        low = m_stats.min - 0.5;
        high = m_stats.max + 0.5;
    }
    m_contrast->setDisplayRange(low, high);
    m_low = m_contrast->low();
    m_high = m_contrast->high();
    m_rangeValid = true;

    m_histogram->setHistogram(m_hist);
    m_histogram->setDisplayRange(m_low, m_high);
    updateStatsLabel();
    setControlsEnabled(true);
    m_stack->setCurrentWidget(m_view);
    render();
    showPixel(-1, -1);
}

void ImagePanel::showPlaceholder(const QString& text)
{
    m_image = Image2D();
    m_rendered = QImage();
    m_stats = ImageStats();
    m_meanStd = processing::MeanStd();
    m_hist = Histogram();
    m_rangeValid = false;
    clearNoise();
    m_view->clearImage();
    m_placeholder->setText(text.isEmpty() ? m_defaultPlaceholder : text);
    m_stack->setCurrentWidget(m_placeholder);
    m_histogram->clear();
    setControlsEnabled(false);
    updateStatsLabel();
    showPixel(-1, -1);
}

void ImagePanel::showBusy(const QString& text)
{
    showPlaceholder(text);
}

void ImagePanel::setSourceInfo(const QString& text, const QString& tooltip)
{
    m_source->setText(text);
    m_source->setToolTip(tooltip.isEmpty() ? text : tooltip);
}

void ImagePanel::setFrameCount(int count)
{
    m_frameCount = std::max(0, count);
    m_frameBar->setVisible(m_frameCount > 0);
    const int n = std::max(1, m_frameCount);
    {
        const QSignalBlocker b1(m_frameSlider);
        const QSignalBlocker b2(m_frameSpin);
        m_frameSlider->setRange(1, n);
        m_frameSpin->setRange(1, n);
        m_frameSlider->setValue(1);
        m_frameSpin->setValue(1);
    }
    m_frameTotal->setText(tr("of %1").arg(n));
    updateFrameControls();
}

void ImagePanel::setFrameMode(FrameMode mode)
{
    if (m_role == PanelRole::Calibrated || mode == m_mode)
        return;
    m_mode = mode;
    {
        const QSignalBlocker b1(m_singleButton);
        const QSignalBlocker b2(m_meanButton);
        m_singleButton->setChecked(mode == FrameMode::Single);
        m_meanButton->setChecked(mode == FrameMode::Mean);
    }
    updateFrameControls();
    updateStatTooltips();
    emit frameModeChanged(mode);
}

void ImagePanel::setAutoCalibrationOff(bool off)
{
    m_autoOff = off;
    if (m_autoOffButton)
        m_autoOffButton->setVisible(off);
    updateStatTooltips();
}

void ImagePanel::updateFrameControls()
{
    // In Mean mode the selector stays visible (so the images in the three
    // panels keep lining up) but is disabled.
    const bool single = m_mode == FrameMode::Single;
    const int v = m_frameSpin->value();
    m_frameLabel->setEnabled(single);
    m_frameSlider->setEnabled(single && m_frameCount > 1);
    m_frameSpin->setEnabled(single && m_frameCount > 1);
    m_prevButton->setEnabled(single && v > m_frameSpin->minimum());
    m_nextButton->setEnabled(single && v < m_frameSpin->maximum());
    m_frameBar->setToolTip(single ? QString() : tr("Mean mode shows the mean over all frames.\n"
                                                   "Switch to Single frame to choose one frame."));
}

void ImagePanel::updateStatTooltips()
{
    QString shown;
    switch (m_role) {
    case PanelRole::Light:
        shown = m_mode == FrameMode::Mean ? QStringLiteral("mean(light, 3)") : QStringLiteral("light(:, :, k)");
        break;
    case PanelRole::Dark:
        shown = m_mode == FrameMode::Mean ? QStringLiteral("mean(dark, 3)") : QStringLiteral("dark(:, :, k)");
        break;
    case PanelRole::Calibrated:
        shown = m_autoOff ? tr("the calibrated image loaded from the file")
                          : tr("light image − dark image (the images on display)");
        break;
    }
    const QString meanTip =
        tr("Mean of the image on display, %1\n\ncalculateImageStats: mean(double(img), \"all\")").arg(shown);
    const QString stdTip = tr("Standard deviation over all pixels of the image on display, %1,\nnormalised by N\n\n"
                              "calculateImageStats: std(double(img), 1, \"all\")")
                               .arg(shown);
    m_meanCaption->setToolTip(meanTip);
    m_meanValue->setToolTip(meanTip);
    m_stdCaption->setToolTip(stdTip);
    m_stdValue->setToolTip(stdTip);
}

int ImagePanel::currentFrame() const
{
    return m_frameSpin->value() - 1;
}

void ImagePanel::setCurrentFrame(int index)
{
    m_frameSpin->setValue(index + 1);  // emits currentFrameChanged if it changes
}

void ImagePanel::stepFrame(int delta)
{
    if (m_frameCount > 0 && m_mode == FrameMode::Single)
        m_frameSpin->setValue(m_frameSpin->value() + delta);  // clamped to the range
}

void ImagePanel::setDisplayRange(double low, double high)
{
    m_low = low;
    m_high = high;
    m_histogram->setDisplayRange(low, high);
    render();
}

void ImagePanel::autoContrast()
{
    if (!hasImage())
        return;
    const auto [lo, hi] = processing::percentileRange(m_image, kAutoLow, kAutoHigh);
    if (std::isfinite(lo) && std::isfinite(hi) && hi > lo)
        m_contrast->applyRange(lo, hi, true);
    else
        fullRangeContrast();
}

void ImagePanel::fullRangeContrast()
{
    if (!hasImage())
        return;
    m_contrast->applyRange(m_stats.min, m_stats.max, true);
}

void ImagePanel::render()
{
    if (m_image.isNull())
        return;

    const int w = m_image.width();
    const int h = m_image.height();
    QImage img(w, h, QImage::Format_Grayscale8);
    const double range = m_high - m_low;
    const double scale = range > 0 ? 255.0 / range : 0.0;
    const double low = m_low;
    const double* src = m_image.constData();
    for (int y = 0; y < h; ++y) {
        uchar* line = img.scanLine(y);
        const double* row = src + qsizetype(y) * w;
        for (int x = 0; x < w; ++x) {
            const double v = (row[x] - low) * scale;
            // NaN fails both comparisons and is drawn black.
            line[x] = !(v > 0.0) ? uchar(0) : (v >= 255.0 ? uchar(255) : uchar(v + 0.5));
        }
    }
    m_rendered = img;
    m_view->setImage(img);
}

QString ImagePanel::noiseText() const
{
    if (m_noisePending)
        return QStringLiteral("…");
    if (m_noiseFrames > 0 && m_noiseFrames < 2)
        return tr("n/a");
    if (std::isnan(m_noise))
        return QStringLiteral("–");
    return formatValue(m_noise);
}

void ImagePanel::updateStatsLabel()
{
    if (!hasImage()) {
        m_meanValue->setText(QStringLiteral("–"));
        m_stdValue->setText(QStringLiteral("–"));
        m_rangeLabel->setText(QStringLiteral(" "));
        m_rangeLabel->setToolTip(QString());
        return;
    }
    m_meanValue->setText(formatValue(m_meanStd.mean));
    m_stdValue->setText(formatValue(m_meanStd.std));
    QString range = tr("Min %1 · Max %2 · %3×%4 px")
                        .arg(formatValue(m_stats.min), formatValue(m_stats.max))
                        .arg(m_image.height())
                        .arg(m_image.width());
    if (m_stats.nonFiniteCount > 0)
        range += tr(" · %1 NaN/Inf pixels (Mean and Std are then NaN, as in MATLAB)").arg(m_stats.nonFiniteCount);
    m_rangeLabel->setText(range);
    m_rangeLabel->setToolTip(range);
}

void ImagePanel::setNoisePending()
{
    m_noise = std::numeric_limits<double>::quiet_NaN();
    m_noisePending = true;
    m_noiseValue->setText(noiseText());
    m_noiseValue->setToolTip(tr("Calculating the temporal noise …"));
}

void ImagePanel::clearNoise()
{
    m_noise = std::numeric_limits<double>::quiet_NaN();
    m_noiseFrames = 0;
    m_noisePending = false;
    m_noiseValue->setText(noiseText());
    m_noiseValue->setToolTip(tr("Temporal noise of the frames behind this panel (calculateImageNoise)"));
}

void ImagePanel::setNoise(double noise, int frameCount)
{
    m_noise = noise;
    m_noiseFrames = frameCount;
    m_noisePending = false;
    m_noiseValue->setText(noiseText());
    if (frameCount < 2) {
        m_noiseValue->setToolTip(tr("Temporal noise needs at least 2 frames.\n"
                                    "(calculateImageNoise returns 0 for a single frame.)"));
        return;
    }

    QString tip;
    switch (m_role) {
    case PanelRole::Light:
        tip = tr("Temporal noise of all %1 light frames\n\n"
                 "calculateImageNoise(light):\nsqrt(mean(var(double(light), 1, 3), \"all\"))\n\n"
                 "It describes the whole stack, so it does not change with the\n"
                 "frame on display or with Single frame / Mean.");
        break;
    case PanelRole::Dark:
        tip = tr("Temporal noise of all %1 dark frames\n\n"
                 "calculateImageNoise(dark):\nsqrt(mean(var(double(dark), 1, 3), \"all\"))\n\n"
                 "It describes the whole stack, so it does not change with the\n"
                 "frame on display or with Single frame / Mean.");
        break;
    case PanelRole::Calibrated:
        if (m_autoOff)
            tip = tr("Temporal noise of the %1 frames loaded from the file\n\n"
                     "calculateImageNoise(calibrated):\nsqrt(mean(var(double(calibrated), 1, 3), \"all\"))");
        else
            tip = tr("Temporal noise of the %1 calibrated frames light(:, :, k) − D, where D is the\n"
                     "dark image on display (mean(dark, 3) or one dark frame)\n\n"
                     "calculateImageNoise(light − D):\n"
                     "sqrt(mean(var(light − D, 1, 3), \"all\"))\n\n"
                     "Subtracting the same image D from every light frame does not change\n"
                     "the temporal variance, so this equals the light-frame noise.");
        break;
    }
    m_noiseValue->setToolTip(tip.arg(frameCount));
}

void ImagePanel::showPixel(int x, int y)
{
    if (!hasImage() || x < 0 || y < 0 || x >= m_image.width() || y >= m_image.height()) {
        m_pixelLabel->setText(hasImage() ? tr("Hover to read pixel values · wheel to zoom · double-click to fit · right-click to copy")
                                         : QString(" "));
        return;
    }
    m_pixelLabel->setText(tr("Row %1, Col %2:  %3").arg(y + 1).arg(x + 1).arg(formatValue(m_image.at(x, y))));
}

void ImagePanel::setControlsEnabled(bool on)
{
    m_contrast->setEnabled(on);
    m_logCheck->setEnabled(on);
    m_autoButton->setEnabled(on);
    m_fullButton->setEnabled(on);
    m_saveMatButton->setEnabled(on);
    m_savePngButton->setEnabled(on);
}

// ---------------------------------------------------------------------------
// Clipboard and report image
// ---------------------------------------------------------------------------

QImage ImagePanel::renderReport() const
{
    if (!hasImage() || m_rendered.isNull())
        return {};

    // Content width: the image at full resolution (within limits).
    const int imgW = m_rendered.width();
    const int imgH = m_rendered.height();
    const int W = std::clamp(imgW, 720, 1600);
    const double s = double(W) / imgW;
    const int dispH = std::max(1, int(std::lround(imgH * s)));

    // The histogram is drawn exactly as it looks in the panel, scaled up to
    // width W. Text and spacing use the same factor, so the whole figure
    // keeps the panel's proportions.
    const QSizeF histShown = m_histogram->size().isEmpty() ? QSizeF(m_histogram->sizeHint())
                                                           : QSizeF(m_histogram->size());
    const double k = double(W) / histShown.width();
    const int histH = int(std::ceil(histShown.height() * k));
    auto scaled = [k](const QFont& f, int extraPx = 0, bool bold = false) {
        QFont out = f;
        out.setPixelSize(std::max(8, int(std::lround((QFontInfo(f).pixelSize() + extraPx) * k))));
        out.setBold(bold || f.bold());
        return out;
    };
    auto gap = [k](double px) { return int(std::lround(px * k)); };

    const QFont titleFont = scaled(m_title->font());
    const QFont base = scaled(m_source->font());
    const QFont small = scaled(font(), -1);
    const QFont valueFont = scaled(m_meanValue->font());
    const QFontMetrics fmTitle(titleFont), fmBase(base), fmSmall(small), fmValue(valueFont);
    const int margin = gap(10);

    const int height = margin + gap(6) + fmTitle.height() + gap(2) + fmBase.height() + gap(6) + dispH + gap(6)
                       + histH + gap(8) + fmSmall.height() + fmValue.height() + gap(4) + fmSmall.height() + margin;
    QImage out(W + 2 * margin, height, QImage::Format_RGB32);
    out.fill(Qt::white);

    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const QColor ink(0x20, 0x20, 0x20);
    const QColor muted(0x6B, 0x6B, 0x6B);

    int y = margin;
    p.setPen(Qt::NoPen);
    p.setBrush(accentFor(m_role));
    p.drawRoundedRect(QRectF(margin, y, W, gap(4)), gap(2), gap(2));
    p.setBrush(Qt::NoBrush);
    y += gap(6);

    p.setFont(titleFont);
    p.setPen(ink);
    p.drawText(QRect(margin, y, W, fmTitle.height()), Qt::AlignLeft | Qt::AlignVCenter, title());
    y += fmTitle.height() + gap(2);
    p.setFont(base);
    p.setPen(muted);
    p.drawText(QRect(margin, y, W, fmBase.height()), Qt::AlignLeft | Qt::AlignVCenter,
               fmBase.elidedText(sourceInfo(), Qt::ElideMiddle, W));
    y += fmBase.height() + gap(6);

    p.setRenderHint(QPainter::SmoothPixmapTransform, s < 1.0);
    p.drawImage(QRect(margin, y, W, dispH), m_rendered);
    y += dispH + gap(6);

    m_histogram->paintAsShown(p, QPointF(margin, y), W);
    y += histH + gap(8);

    const QString captions[3] = {tr("Mean [DN]"), tr("Std [DN]"), tr("Noise [DN]")};
    const QString values[3] = {formatValue(m_meanStd.mean), formatValue(m_meanStd.std), noiseText()};
    const int colW = W / 3;
    for (int i = 0; i < 3; ++i) {
        const int x = margin + i * colW;
        p.setFont(small);
        p.setPen(muted);
        p.drawText(QRect(x, y, colW, fmSmall.height()), Qt::AlignLeft | Qt::AlignVCenter, captions[i]);
        p.setFont(valueFont);
        p.setPen(ink);
        p.drawText(QRect(x, y + fmSmall.height(), colW, fmValue.height()), Qt::AlignLeft | Qt::AlignVCenter, values[i]);
    }
    y += fmSmall.height() + fmValue.height() + gap(4);

    QString note = tr("Display range %1 to %2 · Min %3 · Max %4 · %5×%6 px")
                       .arg(formatValue(m_low), formatValue(m_high), formatValue(m_stats.min), formatValue(m_stats.max))
                       .arg(m_image.height())
                       .arg(m_image.width());
    if (m_noiseFrames >= 2)
        note += tr(" · noise over %1 frames").arg(m_noiseFrames);
    p.setFont(small);
    p.setPen(muted);
    p.drawText(QRect(margin, y, W, fmSmall.height()), Qt::AlignLeft | Qt::AlignVCenter,
               fmSmall.elidedText(note, Qt::ElideRight, W));
    return out;
}

void ImagePanel::copyImageToClipboard()
{
    if (!hasImage())
        return;
    QApplication::clipboard()->setImage(m_rendered.convertToFormat(QImage::Format_RGB32));
    emit message(tr("Copied the %1 image to the clipboard (%2 × %3 px)")
                     .arg(title().toLower())
                     .arg(m_rendered.width())
                     .arg(m_rendered.height()));
}

void ImagePanel::copyReportToClipboard()
{
    if (!hasImage())
        return;
    // Image only, so every application pastes it as a figure.
    const QImage figure = renderReport();
    QApplication::clipboard()->setImage(figure);
    emit message(tr("Copied the %1 figure (image, histogram and statistics) to the clipboard (%2 × %3 px)")
                     .arg(title().toLower())
                     .arg(figure.width())
                     .arg(figure.height()));
}

void ImagePanel::contextMenuEvent(QContextMenuEvent* event)
{
    showContextMenu(event->globalPos());
    event->accept();
}

void ImagePanel::showContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);
    menu.addAction(tr("Copy image"), this, &ImagePanel::copyImageToClipboard);
    menu.addAction(tr("Copy image with histogram and statistics"), this, &ImagePanel::copyReportToClipboard);
    menu.addSeparator();
    menu.addAction(tr("Save image as PNG…"), this, [this] { emit exportRequested(ExportKind::ImagePng); });
    menu.addAction(tr("Save image on display as .mat…"), this, [this] { emit exportRequested(ExportKind::FrameMat); });
    menu.addAction(tr("Save all frames as .mat…"), this, [this] { emit exportRequested(ExportKind::StackMat); });
    menu.addSeparator();
    menu.addAction(tr("Fit to window"), m_view, &ImageView::fitToWindowByUser);

    const bool enabled = hasImage();
    for (QAction* a : menu.actions())
        a->setEnabled(enabled);
    menu.exec(globalPos);
}

// ---------------------------------------------------------------------------
// Drag and drop
// ---------------------------------------------------------------------------

QStringList ImagePanel::droppedPaths(const QMimeData* mime, QString* error) const
{
    if (!mime || !mime->hasUrls())
        return {};
    QStringList local;
    for (const QUrl& url : mime->urls())
        if (url.isLocalFile())
            local << url.toLocalFile();
    if (local.isEmpty())
        return {};
    // Several files only for images, and only in the light and dark panels.
    const FileSelection selection = selectFiles(local, m_role != PanelRole::Calibrated);
    if (!selection.ok()) {
        if (error)
            *error = selection.error;
        return {};
    }
    return selection.files;
}

void ImagePanel::setDropHighlight(bool on)
{
    setProperty("dropActive", on);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void ImagePanel::dragEnterEvent(QDragEnterEvent* event)
{
    QString error;
    if (!droppedPaths(event->mimeData(), &error).isEmpty()) {
        event->acceptProposedAction();
        setDropHighlight(true);
        return;
    }
    if (!error.isEmpty())
        emit message(error);  // say why the drop is refused
    event->ignore();
}

void ImagePanel::dragLeaveEvent(QDragLeaveEvent* event)
{
    setDropHighlight(false);
    QFrame::dragLeaveEvent(event);
}

void ImagePanel::dropEvent(QDropEvent* event)
{
    setDropHighlight(false);
    QString error;
    const QStringList paths = droppedPaths(event->mimeData(), &error);
    if (paths.isEmpty()) {
        if (!error.isEmpty())
            emit message(error);
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    emit filesDropped(paths);
}

} // namespace iat
