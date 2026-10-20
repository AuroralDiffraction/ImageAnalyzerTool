#include "ui/ImagePanel.h"

#include "core/Processing.h"
#include "ui/ContrastControl.h"
#include "ui/HistogramWidget.h"
#include "ui/ImageView.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFontInfo>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
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
        return QObject::tr("Drop a .mat file with light frames here\n"
                           "(an X × Y × Z array: Z images of X × Y pixels)\n\n"
                           "or click Load…");
    case PanelRole::Dark:
        return QObject::tr("Drop a .mat file with dark frames here\n\n"
                           "The pixel-by-pixel mean over all Z frames\n"
                           "becomes the dark frame.\n\n"
                           "or click Load…");
    case PanelRole::Calibrated:
        return QObject::tr("Load light frames and dark frames.\n\n"
                           "This panel then shows\n"
                           "selected light frame − mean dark frame.");
    }
    return {};
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
    setAcceptDrops(role != PanelRole::Calibrated);
    setMinimumWidth(300);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);

    // Accent strip + title row.
    auto* accent = new QFrame(this);
    accent->setFixedHeight(4);
    accent->setStyleSheet(QStringLiteral("background: %1; border-radius: 2px;").arg(accentFor(role).name()));
    layout->addWidget(accent);

    auto* header = new QHBoxLayout;
    m_title = new QLabel(titleFor(role), this);
    QFont tf = m_title->font();
    tf.setBold(true);
    tf.setPointSizeF(tf.pointSizeF() + 2);
    m_title->setFont(tf);
    header->addWidget(m_title);
    header->addStretch(1);
    m_loadButton = new QToolButton(this);
    m_loadButton->setText(tr("Load…"));
    m_loadButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_loadButton->setToolTip(tr("Open a .mat file (or drop one onto this panel)"));
    connect(m_loadButton, &QToolButton::clicked, this, &ImagePanel::loadRequested);
    header->addWidget(m_loadButton);
    if (role == PanelRole::Calibrated) {
        // Keep the space so the three images line up.
        QSizePolicy sp = m_loadButton->sizePolicy();
        sp.setRetainSizeWhenHidden(true);
        m_loadButton->setSizePolicy(sp);
        m_loadButton->setVisible(false);
    }
    layout->addLayout(header);

    m_source = new QLabel(this);
    m_source->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_source->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mute(m_source);
    m_source->setText(role == PanelRole::Calibrated ? tr("Light frame − mean dark frame") : tr("No file loaded"));
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

    // Frame selector (light frames only): Frame [◀] ----slider---- [▶] [ n ] of N
    m_frameBar = new QWidget(this);
    auto* fb = new QHBoxLayout(m_frameBar);
    fb->setContentsMargins(0, 0, 0, 0);
    fb->addWidget(new QLabel(tr("Frame"), m_frameBar));
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
        updateFrameButtons();
        emit currentFrameChanged(v - 1);
    });
    connect(m_frameSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        const QSignalBlocker block(m_frameSlider);
        m_frameSlider->setValue(v);
        updateFrameButtons();
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
    auto addStat = [&](int column, const QString& caption, QLabel*& value, const QString& tip) {
        auto* c = new QLabel(caption, this);
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
        c->setToolTip(tip);
        value->setToolTip(tip);
        stats->addWidget(c, 0, column);
        stats->addWidget(value, 1, column);
    };
    const QString shown = role == PanelRole::Light ? QStringLiteral("light(:, :, k)")
                        : role == PanelRole::Dark  ? QStringLiteral("mean(dark, 3)")
                                                   : QStringLiteral("light(:, :, k) − mean(dark, 3)");
    addStat(0, tr("Mean [DN]"), m_meanValue,
            tr("Mean of the image on display, %1\n\ncalculateImageStats: mean(double(img), \"all\")").arg(shown));
    addStat(1, tr("Std [DN]"), m_stdValue,
            tr("Standard deviation over all pixels of the image on display, %1,\nnormalised by N\n\n"
               "calculateImageStats: std(double(img), 1, \"all\")").arg(shown));
    addStat(2, tr("Noise [DN]"), m_noiseValue, QString());
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
    if (role == PanelRole::Dark) {
        m_saveMatButton->setToolTip(tr("Save the mean dark frame (raw values, double) with Mean, Std and Noise to a .mat file"));
        connect(m_saveMatButton, &QToolButton::clicked, this, [this] { emit exportRequested(ExportKind::FrameMat); });
    } else {
        m_saveMatButton->setToolTip(tr("Save raw values (double) with Mean, Std and Noise to a .mat file"));
        auto* menu = new QMenu(m_saveMatButton);
        menu->addAction(tr("This frame…"), this, [this] { emit exportRequested(ExportKind::FrameMat); });
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

    showPlaceholder(m_defaultPlaceholder);
}

QString ImagePanel::title() const { return m_title->text(); }
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
    m_frameBar->setVisible(count > 0);
    const int n = std::max(1, count);
    {
        const QSignalBlocker b1(m_frameSlider);
        const QSignalBlocker b2(m_frameSpin);
        m_frameSlider->setRange(1, n);
        m_frameSpin->setRange(1, n);
        m_frameSlider->setValue(1);
        m_frameSpin->setValue(1);
    }
    m_frameSlider->setEnabled(n > 1);
    m_frameSpin->setEnabled(n > 1);
    m_frameTotal->setText(tr("of %1").arg(n));
    updateFrameButtons();
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
    if (m_frameBar->isVisible())
        m_frameSpin->setValue(m_frameSpin->value() + delta);  // clamped to the range
}

void ImagePanel::updateFrameButtons()
{
    const int v = m_frameSpin->value();
    m_prevButton->setEnabled(v > m_frameSpin->minimum());
    m_nextButton->setEnabled(v < m_frameSpin->maximum());
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
                 "It does not change when you pick another frame.");
        break;
    case PanelRole::Dark:
        tip = tr("Temporal noise of all %1 dark frames\n\n"
                 "calculateImageNoise(dark):\nsqrt(mean(var(double(dark), 1, 3), \"all\"))");
        break;
    case PanelRole::Calibrated:
        tip = tr("Temporal noise of the %1 calibrated frames light(:, :, k) − mean(dark, 3)\n\n"
                 "calculateImageNoise(light − mean(dark, 3)):\n"
                 "sqrt(mean(var(light − mean(dark, 3), 1, 3), \"all\"))\n\n"
                 "Subtracting the same dark frame from every light frame does not change\n"
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
    if (m_role == PanelRole::Dark) {
        menu.addAction(tr("Save mean dark frame as .mat…"), this, [this] { emit exportRequested(ExportKind::FrameMat); });
    } else {
        menu.addAction(tr("Save this frame as .mat…"), this, [this] { emit exportRequested(ExportKind::FrameMat); });
        menu.addAction(tr("Save all frames as .mat…"), this, [this] { emit exportRequested(ExportKind::StackMat); });
    }
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

QString ImagePanel::matPathFrom(const QMimeData* mime)
{
    if (!mime || !mime->hasUrls())
        return {};
    const QList<QUrl> urls = mime->urls();
    for (const QUrl& url : urls) {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (path.endsWith(QLatin1String(".mat"), Qt::CaseInsensitive))
            return path;
    }
    return {};
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
    if (m_role != PanelRole::Calibrated && !matPathFrom(event->mimeData()).isEmpty()) {
        event->acceptProposedAction();
        setDropHighlight(true);
        return;
    }
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
    const QString path = matPathFrom(event->mimeData());
    if (path.isEmpty()) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    emit fileDropped(path);
}

} // namespace iat
