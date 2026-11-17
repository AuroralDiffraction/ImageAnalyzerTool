#include "ui/PipelineDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace iat {

namespace {

constexpr int kNodeHeight = 96;
constexpr int kSliderSteps = 1000;

// Width of a node: its box must hold the longest word of any stage name
// next to the on / off switch, whatever the font and language.
int nodeWidth(const QFont& font)
{
    QFont bold = font;
    bold.setBold(true);
    const QFontMetrics fm(bold);
    int widest = 0;
    for (const isp::StageSpec& spec : isp::stages())
        for (const QString& word : isp::text(spec.shortName).split(QLatin1Char(' ')))
            widest = std::max(widest, fm.horizontalAdvance(word));
    return std::max(100, widest + 66);
}

QColor mutedColor(const QPalette& pal) { return pal.color(QPalette::PlaceholderText); }

QColor wireColor(const QPalette& pal)
{
    QColor c = pal.color(QPalette::WindowText);
    c.setAlpha(150);
    return c;
}

void mute(QLabel* label)
{
    label->setForegroundRole(QPalette::PlaceholderText);  // follows a change of theme
}

int decimalsFor(const isp::ParamSpec& p)
{
    if (p.kind != isp::ParamKind::Real)
        return 0;
    if (p.logScale)
        return p.min < 0.1 ? 3 : p.min < 1.0 ? 2 : p.min < 10.0 ? 1 : 0;
    int d = 0;
    double step = p.step;
    while (d < 6 && std::abs(step - std::round(step)) > 1e-9) {
        step *= 10.0;
        ++d;
    }
    return d;
}

int toSlider(const isp::ParamSpec& p, double value)
{
    if (p.kind == isp::ParamKind::Integer)
        return int(std::lround(value));
    const double t = p.logScale ? std::log(value / p.min) / std::log(p.max / p.min) : (value - p.min) / (p.max - p.min);
    return int(std::lround(std::clamp(t, 0.0, 1.0) * kSliderSteps));
}

double fromSlider(const isp::ParamSpec& p, int position)
{
    if (p.kind == isp::ParamKind::Integer)
        return position;
    const double t = double(position) / kSliderSteps;
    if (p.logScale)
        return p.min * std::pow(p.max / p.min, t);
    double v = p.min + t * (p.max - p.min);
    if (p.step > 0.0)
        v = p.min + std::round((v - p.min) / p.step) * p.step;
    return std::clamp(v, p.min, p.max);
}

// The end points of the chain: a dot with a label and a piece of wire.
class TerminalNode : public QWidget {
public:
    TerminalNode(const QString& text, bool isInput, QWidget* parent)
        : QWidget(parent)
        , m_text(text)
        , m_input(isInput)
    {
        setFixedSize(std::max(64, fontMetrics().horizontalAdvance(text) + 24), kNodeHeight);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const double wy = 8 + 28;
        const double cx = m_input ? width() - 22.0 : 22.0;
        p.setPen(QPen(wireColor(palette()), 2));
        if (m_input)
            p.drawLine(QPointF(cx, wy), QPointF(width(), wy));
        else
            p.drawLine(QPointF(0, wy), QPointF(cx, wy));
        p.setPen(Qt::NoPen);
        p.setBrush(palette().color(QPalette::WindowText));
        p.drawEllipse(QPointF(cx, wy), 6, 6);
        p.setPen(palette().color(QPalette::WindowText));
        QFont f = font();
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRectF(0, wy + 12, width(), 20), Qt::AlignHCenter | Qt::AlignTop, m_text);
    }

private:
    QString m_text;
    bool m_input;
};

} // namespace

// One stage of the pipeline, drawn as a box on the wire. A stage that is
// turned off is drawn with a dashed outline and the wire passes underneath.
class StageNode : public QWidget {
    Q_OBJECT
public:
    StageNode(const isp::StageSpec& spec, QWidget* parent)
        : QWidget(parent)
        , m_spec(spec)
    {
        setObjectName(QStringLiteral("stageNode_") + QString::fromLatin1(spec.key));
        setFixedSize(nodeWidth(font()), kNodeHeight);
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::PointingHandCursor);
        setToolTip(isp::text(spec.name) + QStringLiteral("\n\n") + isp::text(spec.description));
        setAccessibleName(isp::text(spec.name));
    }

    const isp::StageSpec& spec() const { return m_spec; }
    bool isOn() const { return m_on; }

    void setState(bool on, const QString& subtitle, bool selected)
    {
        if (on == m_on && subtitle == m_subtitle && selected == m_selected)
            return;
        m_on = on;
        m_subtitle = subtitle;
        m_selected = selected;
        update();
    }

signals:
    void selected();
    void toggled(bool on);

protected:
    QRectF boxRect() const { return QRectF(9, 8, width() - 18, 56); }
    QRectF switchRect() const
    {
        const QRectF b = boxRect();
        return QRectF(b.right() - 31, b.top() + 6, 25, 13);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton)
            return;
        emit selected();
        if (switchRect().adjusted(-5, -5, 5, 5).contains(event->position()))
            emit toggled(!m_on);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && !switchRect().adjusted(-5, -5, 5, 5).contains(event->position()))
            emit toggled(!m_on);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Space) {
            emit selected();
            emit toggled(!m_on);
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            emit selected();
            return;
        }
        QWidget::keyPressEvent(event);
    }

    void focusInEvent(QFocusEvent* event) override
    {
        QWidget::focusInEvent(event);
        if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason)
            emit selected();
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QPalette pal = palette();
        const QColor ink = pal.color(QPalette::WindowText);
        const QColor muted = mutedColor(pal);
        const QColor accent = pal.color(QPalette::Highlight);
        const QRectF box = boxRect();
        const double wy = box.center().y();

        // The wire: into and out of the box, or around it when bypassed.
        p.setPen(QPen(wireColor(pal), 2, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        if (m_on) {
            p.drawLine(QPointF(0, wy), QPointF(box.left(), wy));
            p.drawLine(QPointF(box.right(), wy), QPointF(width(), wy));
        } else {
            const double by = box.bottom() + 12;
            QPainterPath path;
            path.moveTo(0, wy);
            path.cubicTo(box.left() - 2, wy, box.left() - 8, by, box.left() + 10, by);
            path.lineTo(box.right() - 10, by);
            path.cubicTo(box.right() + 8, by, box.right() + 2, wy, width(), wy);
            p.drawPath(path);
        }

        // The box.
        QColor fill = pal.color(QPalette::Base);
        if (!m_on)
            fill.setAlpha(90);
        QPen border(m_on ? ink : muted, m_on ? 1.5 : 1.2, m_on ? Qt::SolidLine : Qt::DashLine);
        if (m_selected)
            border = QPen(accent, 2.5, m_on ? Qt::SolidLine : Qt::DashLine);
        p.setPen(border);
        p.setBrush(fill);
        p.drawRoundedRect(box, 7, 7);
        if (m_on) {
            // Arrow head where the signal enters.
            p.setPen(Qt::NoPen);
            p.setBrush(wireColor(pal));
            QPolygonF head;
            head << QPointF(box.left() - 1, wy) << QPointF(box.left() - 8, wy - 4.5) << QPointF(box.left() - 8, wy + 4.5);
            p.drawPolygon(head);
        }
        if (hasFocus()) {
            p.setPen(QPen(accent, 1, Qt::DotLine));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(box.adjusted(-3.5, -3.5, 3.5, 3.5), 9, 9);
        }

        // On / off switch.
        const QRectF sw = switchRect();
        p.setPen(Qt::NoPen);
        p.setBrush(m_on ? accent : muted);
        p.drawRoundedRect(sw, sw.height() / 2, sw.height() / 2);
        p.setBrush(pal.color(QPalette::Base));
        const double r = sw.height() / 2 - 2;
        p.drawEllipse(QPointF(m_on ? sw.right() - r - 2 : sw.left() + r + 2, sw.center().y()), r, r);

        // Stage name (up to two lines) and what it does now.
        QFont nameFont = font();
        nameFont.setBold(true);
        p.setFont(nameFont);
        p.setPen(m_on ? ink : muted);
        const double nameRight = sw.left() - 4;
        const QRectF nameRect(box.left() + 8, box.top() + 3, nameRight - box.left() - 8, 34);
        p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, isp::text(m_spec.shortName));

        QFont subFont = font();
        subFont.setPointSizeF(std::max(7.0, subFont.pointSizeF() - 1.0));
        p.setFont(subFont);
        p.setPen(m_on ? ink : muted);
        const QRectF subRect(box.left() + 8, box.bottom() - 19, box.width() - 14, 16);
        p.drawText(subRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QFontMetrics(subFont).elidedText(m_subtitle, Qt::ElideRight, int(subRect.width())));
    }

private:
    const isp::StageSpec& m_spec;
    bool m_on = false;
    bool m_selected = false;
    QString m_subtitle;
};

// ---------------------------------------------------------------------------

PipelineDialog::PipelineDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Image processing"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setSizeGripEnabled(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    auto* intro = new QLabel(tr("The image passes through the stages from left to right. Use the switch on a "
                                "stage to turn it on or to bypass it; click a stage to choose its algorithm and "
                                "settings."),
                             this);
    intro->setWordWrap(true);
    mute(intro);
    layout->addWidget(intro);

    // The chain of nodes.
    auto* chain = new QWidget(this);
    auto* row = new QHBoxLayout(chain);
    row->setContentsMargins(4, 0, 4, 0);
    row->setSpacing(0);
    row->addWidget(new TerminalNode(tr("Input"), true, chain));
    for (const isp::StageSpec& spec : isp::stages()) {
        auto* node = new StageNode(spec, chain);
        const isp::StageId id = spec.id;
        connect(node, &StageNode::selected, this, [this, id] { selectStage(id); });
        connect(node, &StageNode::toggled, this, [this, id](bool on) { setStageEnabled(id, on); });
        m_nodes.append(node);
        row->addWidget(node);
    }
    row->addWidget(new TerminalNode(tr("Output"), false, chain));
    row->addStretch(1);

    m_strip = new QScrollArea(this);
    m_strip->setObjectName(QStringLiteral("pipelineStrip"));
    m_strip->setWidget(chain);
    m_strip->setWidgetResizable(true);
    m_strip->setFrameShape(QFrame::StyledPanel);
    m_strip->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_strip->setFixedHeight(kNodeHeight + m_strip->horizontalScrollBar()->sizeHint().height() + 6);
    layout->addWidget(m_strip);

    // Settings of the selected stage.
    m_editor = new QFrame(this);
    m_editor->setObjectName(QStringLiteral("stageEditor"));
    auto* ed = new QVBoxLayout(m_editor);
    ed->setContentsMargins(4, 4, 4, 4);
    ed->setSpacing(6);

    auto* head = new QHBoxLayout;
    m_stageTitle = new QLabel(m_editor);
    QFont tf = m_stageTitle->font();
    tf.setBold(true);
    tf.setPointSizeF(tf.pointSizeF() + 2);
    m_stageTitle->setFont(tf);
    head->addWidget(m_stageTitle);
    head->addSpacing(16);
    m_enableCheck = new QCheckBox(tr("Enabled"), m_editor);
    m_enableCheck->setObjectName(QStringLiteral("stageEnabled"));
    m_enableCheck->setToolTip(tr("Turn this stage on. When it is off, the image passes through unchanged (bypass)."));
    head->addWidget(m_enableCheck);
    head->addStretch(1);
    ed->addLayout(head);

    m_stageDescription = new QLabel(m_editor);
    m_stageDescription->setWordWrap(true);
    mute(m_stageDescription);
    ed->addWidget(m_stageDescription);

    auto* algRow = new QHBoxLayout;
    m_algorithmLabel = new QLabel(tr("Algorithm"), m_editor);
    m_algorithmCombo = new QComboBox(m_editor);
    m_algorithmCombo->setObjectName(QStringLiteral("stageAlgorithm"));
    m_algorithmCombo->setMinimumWidth(260);
    m_algorithmLabel->setBuddy(m_algorithmCombo);
    algRow->addWidget(m_algorithmLabel);
    algRow->addWidget(m_algorithmCombo);
    algRow->addStretch(1);
    ed->addLayout(algRow);

    m_algorithmDescription = new QLabel(m_editor);
    m_algorithmDescription->setWordWrap(true);
    ed->addWidget(m_algorithmDescription);

    m_paramBox = new QWidget(m_editor);
    m_paramGrid = new QGridLayout(m_paramBox);
    m_paramGrid->setContentsMargins(0, 4, 0, 0);
    m_paramGrid->setHorizontalSpacing(10);
    m_paramGrid->setVerticalSpacing(6);
    ed->addWidget(m_paramBox);

    m_note = new QLabel(m_editor);
    m_note->setWordWrap(true);
    m_note->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mute(m_note);
    ed->addWidget(m_note);
    ed->addStretch(1);
    layout->addWidget(m_editor, 1);

    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // Bottom row.
    auto* bottom = new QHBoxLayout;
    m_presetButton = new QToolButton(this);
    m_presetButton->setObjectName(QStringLiteral("presetButton"));
    m_presetButton->setText(tr("Presets"));
    m_presetButton->setToolTip(tr("Ready-made settings for common situations. A preset replaces all settings."));
    m_presetButton->setPopupMode(QToolButton::MenuButtonPopup);  // the arrow gets its own part of the button
    connect(m_presetButton, &QToolButton::clicked, m_presetButton, &QToolButton::showMenu);
    {
        auto* menu = new QMenu(m_presetButton);
        menu->setToolTipsVisible(true);
        for (const isp::Preset& preset : isp::presets()) {
            QAction* a = menu->addAction(isp::text(preset.name));
            a->setToolTip(isp::text(preset.description));
            const char* id = preset.id;
            connect(a, &QAction::triggered, this, [this, id] { applyPreset(id); });
        }
        m_presetButton->setMenu(menu);
    }
    bottom->addWidget(m_presetButton);
    m_resetStageButton = new QPushButton(tr("Reset stage"), this);
    m_resetStageButton->setAutoDefault(false);
    m_resetStageButton->setToolTip(tr("Default settings for the selected stage, and bypass it"));
    connect(m_resetStageButton, &QPushButton::clicked, this, [this] { resetStage(m_selected); });
    bottom->addWidget(m_resetStageButton);
    auto* resetAll = new QPushButton(tr("Reset all"), this);
    resetAll->setAutoDefault(false);
    resetAll->setToolTip(tr("Default settings for every stage, all bypassed: the image is shown unprocessed"));
    connect(resetAll, &QPushButton::clicked, this, &PipelineDialog::resetAll);
    bottom->addWidget(resetAll);
    bottom->addSpacing(12);
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("pipelineStatus"));
    mute(m_status);
    bottom->addWidget(m_status, 1);
    m_originalCheck = new QCheckBox(tr("Show original"), this);
    m_originalCheck->setObjectName(QStringLiteral("showOriginal"));
    m_originalCheck->setToolTip(tr("Show the unprocessed image, keeping all settings, to compare before and after"));
    connect(m_originalCheck, &QCheckBox::toggled, this, [this](bool on) {
        updateStatus();
        emit showOriginalChanged(on);
    });
    m_abCheck = new QCheckBox(QStringLiteral("A/B"), this);
    m_abCheck->setObjectName(QStringLiteral("abCompare"));
    m_abCheck->setToolTip(tr("A/B comparison: shows a divider on the panel's image. Left of it (A) is the unprocessed\n"
                             "image, right of it (B) the processed one. Drag the divider across the image."));
    connect(m_abCheck, &QCheckBox::toggled, this, &PipelineDialog::abCompareChanged);
    bottom->addWidget(m_abCheck);
    bottom->addSpacing(8);
    bottom->addWidget(m_originalCheck);
    auto* close = new QPushButton(tr("Close"), this);
    close->setAutoDefault(false);
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    bottom->addWidget(close);
    layout->addLayout(bottom);

    connect(m_enableCheck, &QCheckBox::toggled, this, [this](bool on) { setStageEnabled(m_selected, on); });
    connect(m_algorithmCombo, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        const isp::StageSpec& spec = isp::stage(m_selected);
        if (index >= 0 && index < int(spec.algorithms.size()))
            setStageAlgorithm(m_selected, spec.algorithms[std::size_t(index)].id);
    });

    // Wide enough for the whole chain, if the screen allows; it scrolls otherwise.
    int maxWidth = 1500;
    if (const QScreen* scr = parent ? parent->screen() : QGuiApplication::primaryScreen())
        maxWidth = std::min(maxWidth, scr->availableGeometry().width() - 60);
    resize(std::max(640, std::min(maxWidth, chain->sizeHint().width() + 2 * m_strip->frameWidth() + 28)), 540);
    rebuildEditor();
    updateNodes();
    updateStatus();
}

void PipelineDialog::setPipeline(const isp::Pipeline& pipeline)
{
    if (pipeline == m_pipeline)
        return;
    m_pipeline = pipeline;
    m_notes.clear();
    rebuildEditor();
    updateNodes();
    updateStatus();
}

bool PipelineDialog::showOriginal() const { return m_originalCheck->isChecked(); }

void PipelineDialog::setShowOriginal(bool on) { m_originalCheck->setChecked(on); }

bool PipelineDialog::abCompare() const { return m_abCheck->isChecked(); }

void PipelineDialog::setAbCompare(bool on) { m_abCheck->setChecked(on); }

void PipelineDialog::setResult(const QStringList& notes, double milliseconds)
{
    m_notes = notes;
    m_milliseconds = milliseconds;
    const int i = isp::stageIndex(m_selected);
    m_note->setText(m_pipeline.isEnabled(m_selected) && i < m_notes.size() ? m_notes[i] : QString());
    updateStatus();
}

void PipelineDialog::setBusy(bool busy)
{
    m_busy = busy;
    updateStatus();
}

void PipelineDialog::selectStage(isp::StageId id)
{
    if (id == m_selected)
        return;
    m_selected = id;
    rebuildEditor();
    updateNodes();
    m_strip->ensureWidgetVisible(m_nodes[isp::stageIndex(id)], 40, 0);
}

void PipelineDialog::setStageEnabled(isp::StageId id, bool on)
{
    if (m_pipeline.isEnabled(id) == on)
        return;
    m_pipeline.setEnabled(id, on);
    if (id == m_selected) {
        const QSignalBlocker block(m_enableCheck);
        m_enableCheck->setChecked(on);
        m_paramBox->setEnabled(on);
        m_algorithmCombo->setEnabled(on);
        m_algorithmLabel->setEnabled(on);
        m_algorithmDescription->setEnabled(on);
        if (!on)
            m_note->clear();
    }
    changed();
}

void PipelineDialog::setStageAlgorithm(isp::StageId id, const char* algorithmId)
{
    const int before = m_pipeline.config(id).algorithm;
    if (!m_pipeline.setAlgorithm(id, algorithmId) || m_pipeline.config(id).algorithm == before)
        return;
    if (id == m_selected)
        rebuildEditor();
    changed();
}

void PipelineDialog::setStageValue(isp::StageId id, const char* paramId, double value)
{
    const double before = m_pipeline.value(id, paramId);
    if (!m_pipeline.setValue(id, paramId, value) || m_pipeline.value(id, paramId) == before)
        return;
    if (id == m_selected)
        rebuildEditor();
    changed();
}

void PipelineDialog::applyPreset(const char* presetId)
{
    const isp::Pipeline preset = isp::presetPipeline(presetId);
    if (preset == m_pipeline)
        return;
    m_pipeline = preset;
    rebuildEditor();
    changed();
}

void PipelineDialog::resetStage(isp::StageId id)
{
    isp::Pipeline next = m_pipeline;
    next.resetStage(id);
    if (next == m_pipeline)
        return;
    m_pipeline = next;
    if (id == m_selected)
        rebuildEditor();
    changed();
}

void PipelineDialog::resetAll()
{
    if (m_pipeline == isp::Pipeline())
        return;
    m_pipeline = isp::Pipeline();
    rebuildEditor();
    changed();
}

void PipelineDialog::changed()
{
    updateNodes();
    updateStatus();
    emit pipelineChanged(m_pipeline);
}

void PipelineDialog::updateNodes()
{
    for (StageNode* node : m_nodes) {
        const isp::StageSpec& spec = node->spec();
        const bool on = m_pipeline.isEnabled(spec.id);
        QString subtitle;
        if (!on)
            subtitle = tr("Bypassed");
        else if (const isp::AlgorithmSpec* a = m_pipeline.algorithm(spec.id))
            subtitle = isp::text(a->name);
        node->setState(on, subtitle, spec.id == m_selected);
    }
}

void PipelineDialog::updateStatus()
{
    QString text;
    if (showOriginal())
        text = tr("Showing the original image");
    else if (!m_pipeline.isActive())
        text = tr("Every stage is bypassed: the image is shown unprocessed");
    else if (m_busy)
        text = tr("Processing …");
    else if (m_milliseconds >= 0.0)
        text = tr("Processed in %1 ms").arg(QString::number(m_milliseconds, 'f', 0));
    m_status->setText(text);
    // There is something to compare only while a processed image is shown.
    m_abCheck->setEnabled(m_pipeline.isActive() && !showOriginal());
}

void PipelineDialog::rebuildEditor()
{
    const isp::StageSpec& spec = isp::stage(m_selected);
    const isp::StageConfig& cfg = m_pipeline.config(m_selected);
    const bool on = m_pipeline.isEnabled(m_selected);

    m_stageTitle->setText(isp::text(spec.name));
    m_stageDescription->setText(isp::text(spec.description));
    {
        const QSignalBlocker block(m_enableCheck);
        m_enableCheck->setChecked(on);
    }

    {
        const QSignalBlocker block(m_algorithmCombo);
        m_algorithmCombo->clear();
        for (const isp::AlgorithmSpec& a : spec.algorithms)
            m_algorithmCombo->addItem(isp::text(a.name));
        m_algorithmCombo->setCurrentIndex(cfg.algorithm);
    }
    m_algorithmLabel->setEnabled(on);
    m_algorithmCombo->setEnabled(on);
    m_algorithmDescription->setEnabled(on);

    // Parameter rows: label | slider or list | value.
    while (QLayoutItem* item = m_paramGrid->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->hide();
            w->deleteLater();
        }
        delete item;
    }
    const isp::AlgorithmSpec* alg = m_pipeline.algorithm(m_selected);
    m_algorithmDescription->setText(alg ? isp::text(alg->description) : QString());
    m_algorithmDescription->setVisible(alg != nullptr);
    if (alg) {
        const isp::StageId stageId = m_selected;
        int rowIndex = 0;
        for (const isp::ParamSpec& param : alg->params) {
            const double value = m_pipeline.value(stageId, param.id);
            const QString name = QString::fromLatin1(param.id);
            auto* label = new QLabel(isp::text(param.label), m_paramBox);
            m_paramGrid->addWidget(label, rowIndex, 0);
            const char* paramId = param.id;

            if (param.kind == isp::ParamKind::Choice) {
                auto* combo = new QComboBox(m_paramBox);
                combo->setObjectName(QStringLiteral("param_") + name);
                for (const char* c : param.choices)
                    combo->addItem(isp::text(c));
                combo->setCurrentIndex(int(std::lround(value)));
                label->setBuddy(combo);
                connect(combo, qOverload<int>(&QComboBox::activated), this, [this, stageId, paramId](int index) {
                    if (m_pipeline.setValue(stageId, paramId, index))
                        changed();
                });
                m_paramGrid->addWidget(combo, rowIndex, 1, Qt::AlignLeft);
            } else {
                auto* slider = new QSlider(Qt::Horizontal, m_paramBox);
                slider->setObjectName(QStringLiteral("param_") + name);
                if (param.kind == isp::ParamKind::Integer)
                    slider->setRange(int(param.min), int(param.max));
                else
                    slider->setRange(0, kSliderSteps);
                slider->setValue(toSlider(param, value));
                slider->setMinimumWidth(240);
                auto* spin = new QDoubleSpinBox(m_paramBox);
                spin->setObjectName(QStringLiteral("value_") + name);
                spin->setKeyboardTracking(false);
                spin->setDecimals(decimalsFor(param));
                spin->setRange(param.min, param.max);
                spin->setSingleStep(param.logScale ? std::max(std::pow(10.0, -decimalsFor(param)), value / 10.0)
                                                   : param.step);
                spin->setAlignment(Qt::AlignRight);
                spin->setMinimumWidth(110);
                const QString unit = isp::text(param.unit);
                if (!unit.isEmpty())
                    spin->setSuffix(QLatin1Char(' ') + unit);
                spin->setValue(value);
                label->setBuddy(slider);

                // Slider and number box show the same value; either one sets it.
                const isp::ParamSpec* ps = &param;
                connect(slider, &QSlider::valueChanged, this, [this, stageId, paramId, ps, spin](int pos) {
                    if (!m_pipeline.setValue(stageId, paramId, fromSlider(*ps, pos)))
                        return;
                    const QSignalBlocker block(spin);
                    spin->setValue(m_pipeline.value(stageId, paramId));
                    changed();
                });
                connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                        [this, stageId, paramId, ps, slider](double v) {
                            if (!m_pipeline.setValue(stageId, paramId, v))
                                return;
                            const QSignalBlocker block(slider);
                            slider->setValue(toSlider(*ps, m_pipeline.value(stageId, paramId)));
                            changed();
                        });
                m_paramGrid->addWidget(slider, rowIndex, 1);
                m_paramGrid->addWidget(spin, rowIndex, 2);
            }
            ++rowIndex;
        }
        m_paramGrid->setColumnStretch(1, 1);
    }
    m_paramBox->setEnabled(on);
    m_paramBox->setVisible(alg && !alg->params.empty());

    const int i = isp::stageIndex(m_selected);
    m_note->setText(on && i < m_notes.size() ? m_notes[i] : QString());
}

} // namespace iat

#include "PipelineDialog.moc"
