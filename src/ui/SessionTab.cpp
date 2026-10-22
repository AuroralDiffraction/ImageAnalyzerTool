#include "ui/SessionTab.h"

#include "core/Processing.h"
#include "io/ImageFileReader.h"
#include "io/MatFileReader.h"
#include "io/MatFileWriter.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include <cmath>
#include <utility>
#include <vector>

namespace iat {

namespace {

QString formatNoise(double v) { return QString::number(v, 'g', 7); }

// "1 frame", "8 frames"
QString countOf(int n, const QString& one, const QString& many)
{
    return QStringLiteral("%1 %2").arg(n).arg(n == 1 ? one : many);
}

// "6×8" for one frame, "6×8×4" for a stack.
QString sizeOf(const ImageStack& s)
{
    const QString base = QStringLiteral("%1×%2").arg(s.rows()).arg(s.cols());
    return s.frameCount() > 1 ? base + QStringLiteral("×%1").arg(s.frameCount()) : base;
}

// File names as stored in exported .mat files: full paths, one per line.
QString fileList(const QStringList& paths)
{
    QStringList native;
    for (const QString& p : paths)
        native << QDir::toNativeSeparators(p);
    return native.join(QLatin1Char('\n'));
}

struct Stats {
    Image2D mean;
    double noise = std::numeric_limits<double>::quiet_NaN();
};

} // namespace

// Result of reading the files for one panel (on a worker thread).
struct SessionTab::ReadResult {
    bool isMat = false;
    QStringList paths;
    MatFileContents mat;   // a .mat file
    ImageSequence images;  // image files
};

template <typename T, typename Work, typename Done>
void SessionTab::runInBackground(Work work, Done done)
{
    addPending(+1);
    auto* watcher = new QFutureWatcher<T>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, done = std::move(done)] {
        const T result = watcher->result();
        watcher->deleteLater();
        done(result);
        addPending(-1);
    });
    watcher->setFuture(QtConcurrent::run(std::move(work)));
}

SessionTab::SessionTab(QWidget* parent)
    : QWidget(parent)
{
    m_lightPanel = new ImagePanel(PanelRole::Light, this);
    m_darkPanel = new ImagePanel(PanelRole::Dark, this);
    m_calPanel = new ImagePanel(PanelRole::Calibrated, this);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(8);
    for (ImagePanel* p : {m_lightPanel, m_darkPanel, m_calPanel})
        splitter->addWidget(p);
    for (int i = 0; i < 3; ++i)
        splitter->setStretchFactor(i, 1);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(splitter);

    // The calibrated noise is recalculated when the dark image changes. While
    // the dark frame is being stepped through, wait until it settles.
    m_calNoiseTimer = new QTimer(this);
    m_calNoiseTimer->setSingleShot(true);
    m_calNoiseTimer->setInterval(150);
    connect(m_calNoiseTimer, &QTimer::timeout, this, [this] {
        addPending(-1);
        startCalibratedNoise();
    });

    connect(m_lightPanel, &ImagePanel::filesDropped, this, [this](const QStringList& paths) { loadLight(paths); });
    connect(m_darkPanel, &ImagePanel::filesDropped, this, [this](const QStringList& paths) { loadDark(paths); });
    connect(m_calPanel, &ImagePanel::filesDropped, this, [this](const QStringList& paths) {
        if (!paths.isEmpty())
            loadCalibrated(paths.first());
    });
    connect(m_lightPanel, &ImagePanel::loadRequested, this, &SessionTab::openLightDialog);
    connect(m_darkPanel, &ImagePanel::loadRequested, this, &SessionTab::openDarkDialog);
    connect(m_calPanel, &ImagePanel::loadRequested, this, &SessionTab::openCalibratedDialog);
    for (Target t : {Target::Light, Target::Dark}) {
        // A new frame or mode changes this panel's image and the calibrated
        // image; the noise values describe whole stacks and stay as they are.
        connect(panel(t), &ImagePanel::currentFrameChanged, this, [this, t] {
            refreshPanel(t, false);
            updateCalibrated(false);
        });
        connect(panel(t), &ImagePanel::frameModeChanged, this, [this, t] {
            refreshPanel(t, false);
            updateCalibrated(false);
        });
    }
    connect(m_calPanel, &ImagePanel::currentFrameChanged, this, [this] { refreshPanel(Target::Calibrated, false); });
    connect(m_calPanel, &ImagePanel::autoCalibrationRequested, this, &SessionTab::restoreAutoCalibration);
    for (ImagePanel* p : {m_lightPanel, m_darkPanel, m_calPanel}) {
        connect(p, &ImagePanel::exportRequested, this, [this, p](ExportKind kind) { exportFrom(p, kind); });
        connect(p, &ImagePanel::message, this, &SessionTab::statusMessage);
    }
    linkViews();

    updateCalibrated(true);
}

SessionTab::Source& SessionTab::source(Target t)
{
    return t == Target::Light ? m_light : t == Target::Dark ? m_dark : m_cal;
}

const SessionTab::Source& SessionTab::source(Target t) const
{
    return t == Target::Light ? m_light : t == Target::Dark ? m_dark : m_cal;
}

ImagePanel* SessionTab::panel(Target t) const
{
    return t == Target::Light ? m_lightPanel : t == Target::Dark ? m_darkPanel : m_calPanel;
}

QString SessionTab::targetName(Target t) const
{
    switch (t) {
    case Target::Light: return tr("light frames");
    case Target::Dark: return tr("dark frames");
    case Target::Calibrated: break;
    }
    return tr("calibrated frames");
}

bool SessionTab::isAutoCalibration() const
{
    return m_cal.isNull() && !m_reading[int(Target::Calibrated)];
}

// ---------------------------------------------------------------------------
// Linked zoom and pan
// ---------------------------------------------------------------------------

void SessionTab::linkViews()
{
    const ImagePanel* panels[3] = {m_lightPanel, m_darkPanel, m_calPanel};
    for (const ImagePanel* source : panels) {
        ImageView* view = source->view();
        // The user zoomed or panned this view: the others follow.
        connect(view, &ImageView::viewStateChanged, this, [this, view, panels] {
            m_sharedView = view->viewState();
            for (const ImagePanel* other : panels)
                if (other->view() != view)
                    other->view()->setViewState(m_sharedView);
        });
        // A view that starts showing a new image adopts the shared zoom/pan.
        connect(view, &ImageView::imageSizeChanged, this, [this, view] { view->setViewState(m_sharedView); });
    }
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

void SessionTab::loadLight(const QStringList& paths) { startLoad(Target::Light, paths); }
void SessionTab::loadDark(const QStringList& paths) { startLoad(Target::Dark, paths); }
void SessionTab::loadCalibrated(const QString& path) { startLoad(Target::Calibrated, {path}); }

QStringList SessionTab::openDialog(const QString& title, bool multiple)
{
    QSettings settings;
    const QString dir = settings.value(QStringLiteral("lastDirectory"), QDir::homePath()).toString();
    const QString patterns = ImageFileReader::imageFilePatterns();
    const QString filter = tr("MATLAB data and images (*.mat %1);;MATLAB data (*.mat);;Images (%1);;All files (*)")
                               .arg(patterns);
    QStringList paths;
    if (multiple)
        paths = QFileDialog::getOpenFileNames(this, title, dir, filter);
    else
        paths << QFileDialog::getOpenFileName(this, title, dir, filter);
    paths.removeAll(QString());
    if (!paths.isEmpty())
        settings.setValue(QStringLiteral("lastDirectory"), QFileInfo(paths.first()).absolutePath());
    return paths;
}

void SessionTab::openLightDialog()
{
    const QStringList paths = openDialog(tr("Open light frames: one .mat file, or one or more images"), true);
    if (!paths.isEmpty())
        loadLight(paths);
}

void SessionTab::openDarkDialog()
{
    const QStringList paths = openDialog(tr("Open dark frames: one .mat file, or one or more images"), true);
    if (!paths.isEmpty())
        loadDark(paths);
}

void SessionTab::openCalibratedDialog()
{
    const QStringList paths = openDialog(tr("Open a calibrated image (turns automatic calibration off)"), false);
    if (!paths.isEmpty())
        loadCalibrated(paths.first());
}

void SessionTab::addPending(int delta)
{
    const bool wasBusy = isBusy();
    m_pending += delta;
    if (wasBusy != isBusy())
        emit busyChanged(isBusy());
}

void SessionTab::startLoad(Target target, const QStringList& input)
{
    const FileSelection selection = selectFiles(input, target != Target::Calibrated);
    if (!selection.ok()) {
        QMessageBox::warning(this, tr("Cannot load these files"), selection.error);
        return;
    }
    const QStringList paths = selection.files;
    const bool isMat = selection.isMat;

    const quint64 gen = ++m_generation[int(target)];
    m_reading[int(target)] = true;
    const QString what = paths.size() == 1 ? QFileInfo(paths.first()).fileName() : tr("%1 images").arg(paths.size());
    panel(target)->showBusy(tr("Reading %1 …").arg(what));
    QString status = tr("Reading %1 …").arg(paths.size() == 1 ? QDir::toNativeSeparators(paths.first()) : what);
    if (selection.ignored > 0)
        status += tr(" (%1 of another type ignored)").arg(countOf(selection.ignored, tr("file"), tr("files")));
    emit statusMessage(status);

    runInBackground<ReadResult>(
        [paths, isMat] {
            ReadResult r;
            r.isMat = isMat;
            r.paths = paths;
            if (isMat)
                r.mat = MatFileReader::read(paths.first());
            else
                r.images = ImageFileReader::readSequence(paths);
            return r;
        },
        [this, target, gen](const ReadResult& r) {
            if (gen != m_generation[int(target)])
                return;  // a newer load has started, or it was cancelled
            m_reading[int(target)] = false;
            onFileRead(target, r);
        });
}

std::optional<MatVariable> SessionTab::chooseVariable(Target target, const QString& path,
                                                      const MatFileContents& contents)
{
    const QList<MatVariable> stacks = contents.imageStacks();
    if (stacks.size() == 1)
        return stacks.first();

    if (stacks.isEmpty()) {
        QStringList found;
        for (const MatVariable& v : contents.variables)
            found << QStringLiteral("  %1  (%2 %3)%4")
                         .arg(v.name, v.sizeString(), v.matlabClass,
                              v.note.isEmpty() ? QString() : QStringLiteral(": ") + v.note);
        QMessageBox::warning(this, tr("No image data"),
                             tr("%1 does not contain a numeric X × Y or X × Y × Z array.\n\nVariables in the file:\n%2")
                                 .arg(QFileInfo(path).fileName(),
                                      found.isEmpty() ? tr("  (none)") : found.join('\n')));
        return std::nullopt;
    }

    QStringList items;
    for (const MatVariable& v : stacks)
        items << QStringLiteral("%1   (%2 %3)").arg(v.name, v.sizeString(), v.matlabClass);
    bool ok = false;
    const QString item = QInputDialog::getItem(
        this, tr("Choose a variable"),
        tr("%1 contains several arrays.\nWhich one holds the %2?").arg(QFileInfo(path).fileName(), targetName(target)),
        items, 0, false, &ok);
    if (!ok)
        return std::nullopt;
    return stacks.at(int(items.indexOf(item)));
}

void SessionTab::onFileRead(Target target, const ReadResult& r)
{
    Source s;
    s.paths = r.paths;
    if (r.isMat) {
        const QString& path = r.paths.first();
        if (!r.mat.ok()) {
            QMessageBox::warning(this, tr("Could not open file"),
                                 QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), r.mat.error));
            emit statusMessage(tr("Could not open %1").arg(QFileInfo(path).fileName()));
            loadFailed(target);
            return;
        }
        const std::optional<MatVariable> chosen = chooseVariable(target, path, r.mat);
        if (!chosen) {
            loadFailed(target);
            return;
        }
        s.stack = chosen->data;
        s.matlabClass = chosen->matlabClass;
        s.info = QStringLiteral("%1 · %2 · %3 %4")
                     .arg(QFileInfo(path).fileName(), chosen->name, chosen->sizeString(), chosen->matlabClass);
    } else {
        const ImageSequence& seq = r.images;
        if (!seq.ok()) {
            QMessageBox::warning(this, tr("Could not open images"), seq.error);
            emit statusMessage(tr("Could not open the images: %1").arg(seq.error));
            loadFailed(target);
            return;
        }
        s.stack = seq.data;
        s.paths = seq.paths;  // in frame order
        s.matlabClass = seq.matlabClass;
        s.notes = seq.notes;
        const int n = int(seq.paths.size());
        const QString names = n == 1 ? QFileInfo(seq.paths.first()).fileName()
                                     : tr("%1 … %2 · %3 %4 files")
                                           .arg(QFileInfo(seq.paths.first()).fileName(),
                                                QFileInfo(seq.paths.last()).fileName())
                                           .arg(n)
                                           .arg(seq.format == QLatin1String("image") ? tr("image") : seq.format);
        s.info = QStringLiteral("%1 · %2 %3").arg(names, sizeOf(seq.data), seq.matlabClass);
    }
    setSource(target, std::move(s));
}

void SessionTab::loadFailed(Target target)
{
    if (target == Target::Calibrated && m_cal.isNull())
        updateCalibrated(true);  // back to the automatic result
    else
        refreshPanel(target, false);  // show what the panel had before
}

void SessionTab::setSource(Target target, Source s)
{
    s.id = ++m_nextSourceId;
    source(target) = std::move(s);
    const Source& src = source(target);
    ImagePanel* p = panel(target);
    p->setFrameCount(src.frames());  // selects the first frame
    if (target == Target::Calibrated)
        p->setAutoCalibrationOff(true);
    refreshPanel(target, true);
    startStats(target);
    if (target != Target::Calibrated)
        updateCalibrated(true);

    QString msg = target == Target::Calibrated
                      ? tr("Loaded a calibrated image: %1 · automatic calibration is off").arg(src.info)
                      : tr("Loaded %1: %2").arg(targetName(target), src.info);
    if (!src.notes.isEmpty())
        msg += QStringLiteral(" (%1)").arg(src.notes.join(QStringLiteral("; ")));
    emit statusMessage(msg);
}

void SessionTab::startStats(Target target)
{
    const Source& s = source(target);
    if (s.isNull())
        return;
    const quint64 id = s.id;
    const ImageStack stack = s.stack;

    runInBackground<Stats>(
        [stack] {
            Stats r;
            r.mean = processing::meanOverFrames(stack);
            r.noise = processing::calculateImageNoise(stack);
            return r;
        },
        [this, target, id](const Stats& r) {
            Source& src = source(target);
            if (src.id != id)
                return;  // another file was loaded meanwhile
            src.mean = r.mean;
            src.noise = r.noise;
            src.statsReady = true;
            if (m_reading[int(target)])
                return;  // the panel is showing a newer read; keep the values for later
            if (src.shown.isNull()) {
                // The panel was waiting for the mean.
                refreshPanel(target, true);
                if (target != Target::Calibrated)
                    updateCalibrated(true);
            } else {
                panel(target)->setNoise(src.noise, src.frames());
            }
            if (src.frames() >= 2)  // one frame has no temporal noise (the panel shows n/a)
                emit statusMessage(
                    tr("Calculated noise of %1 [DN]: %2").arg(targetName(target), formatNoise(src.noise)));
        });
}

QString SessionTab::panelInfo(Target target) const
{
    const Source& s = source(target);
    QString info = s.info;
    if (target != Target::Calibrated && panel(target)->frameMode() == FrameMode::Mean)
        info += tr(" · mean of %1").arg(countOf(s.frames(), tr("frame"), tr("frames")));
    return info;
}

void SessionTab::refreshPanel(Target target, bool resetContrast)
{
    if (m_reading[int(target)])
        return;  // the panel shows the reading progress
    Source& s = source(target);
    ImagePanel* p = panel(target);
    if (target == Target::Calibrated && s.isNull())
        return;  // automatic calibration: updateCalibrated() fills the panel
    if (s.isNull()) {
        s.shown = Image2D();
        p->showPlaceholder(QString());
        p->setSourceInfo(tr("No file loaded"));
        return;
    }

    QStringList tip;
    for (const QString& path : s.paths)
        tip << QDir::toNativeSeparators(path);
    if (!s.notes.isEmpty())
        tip << QString() << s.notes;
    p->setSourceInfo(panelInfo(target), tip.join(QLatin1Char('\n')));

    const bool mean = p->frameMode() == FrameMode::Mean;
    if (mean && !s.statsReady) {
        s.shown = Image2D();
        p->showBusy(tr("Calculating the mean of %1 …").arg(countOf(s.frames(), tr("frame"), tr("frames"))));
        return;
    }
    s.shown = mean ? s.mean : s.stack.frame(p->currentFrame());
    p->setImage(s.shown, resetContrast);
    if (s.statsReady)
        p->setNoise(s.noise, s.frames());
    else
        p->setNoisePending();
}

// ---------------------------------------------------------------------------
// Calibrated panel
// ---------------------------------------------------------------------------

void SessionTab::restoreAutoCalibration()
{
    ++m_generation[int(Target::Calibrated)];  // drops a read that is still running
    m_reading[int(Target::Calibrated)] = false;
    m_cal = Source();
    m_calPanel->setAutoCalibrationOff(false);
    m_calPanel->setFrameCount(0);
    updateCalibrated(true);
    emit statusMessage(tr("Automatic calibration is on: light image − dark image"));
}

QString SessionTab::calibrationDescription() const
{
    const QString light = m_lightPanel->frameMode() == FrameMode::Mean
                              ? tr("Mean of %1").arg(countOf(m_light.frames(), tr("light frame"), tr("light frames")))
                              : tr("Light frame %1 of %2").arg(m_lightPanel->currentFrame() + 1).arg(m_light.frames());
    const QString dark = m_darkPanel->frameMode() == FrameMode::Mean
                             ? tr("mean of %1").arg(countOf(m_dark.frames(), tr("dark frame"), tr("dark frames")))
                             : tr("dark frame %1 of %2").arg(m_darkPanel->currentFrame() + 1).arg(m_dark.frames());
    return QStringLiteral("%1 − %2").arg(light, dark);
}

QString SessionTab::calibrationFormula(bool wholeStack) const
{
    // MATLAB notation, ASCII only, for the exported .mat files.
    const QString dark = m_darkPanel->frameMode() == FrameMode::Mean
                             ? QStringLiteral("mean(dark, 3)")
                             : QStringLiteral("dark(:, :, %1)").arg(m_darkPanel->currentFrame() + 1);
    QString light;
    if (wholeStack)
        light = QStringLiteral("light");
    else if (m_lightPanel->frameMode() == FrameMode::Mean)
        light = QStringLiteral("mean(light, 3)");
    else
        light = QStringLiteral("light(:, :, %1)").arg(m_lightPanel->currentFrame() + 1);
    return QStringLiteral("%1 - %2").arg(light, dark);
}

void SessionTab::updateCalibrated(bool resetContrast)
{
    if (!isAutoCalibration())
        return;  // the panel shows an image loaded from a file

    const Image2D& lightImage = m_light.shown;
    const Image2D& darkImage = m_dark.shown;
    const bool haveLight = !lightImage.isNull();
    const bool haveDark = !darkImage.isNull();

    if (!haveLight || !haveDark) {
        const bool lightLoaded = !m_light.isNull() && !m_reading[int(Target::Light)];
        const bool darkLoaded = !m_dark.isNull() && !m_reading[int(Target::Dark)];
        QString msg;
        if (!lightLoaded && !darkLoaded)
            msg = QString();  // default text
        else if (!lightLoaded)
            msg = tr("Dark frames ready.\n\nNow load light frames to compute\nlight image − dark image.");
        else if (!darkLoaded)
            msg = tr("Light frames ready.\n\nNow load dark frames to compute\nlight image − dark image.");
        else if (!haveLight && !haveDark)
            msg = tr("Waiting for the mean light and dark images …");
        else
            msg = haveLight ? tr("Waiting for the mean dark image …") : tr("Waiting for the mean light image …");
        m_calPanel->showPlaceholder(msg);
        m_calPanel->setSourceInfo(tr("Light image − dark image"));
        return;
    }

    if (!lightImage.sameSize(darkImage)) {
        m_calPanel->showPlaceholder(
            tr("The sizes do not match:\n\nlight image  %1 × %2\ndark image   %3 × %4\n\n"
               "Load files with the same X × Y size.")
                .arg(lightImage.height()).arg(lightImage.width())
                .arg(darkImage.height()).arg(darkImage.width()));
        m_calPanel->setSourceInfo(tr("Light image − dark image"));
        return;
    }

    const Image2D calibrated = processing::subtract(lightImage, darkImage);
    m_calPanel->setSourceInfo(calibrationDescription());
    m_calPanel->setImage(calibrated, resetContrast || !m_calPanel->hasImage());

    if (m_calNoiseValid && currentNoiseKey() == m_calNoiseKey) {
        m_calPanel->setNoise(m_calNoise, m_light.frames());
    } else {
        m_calPanel->setNoisePending();
        scheduleCalibratedNoise();
    }
}

SessionTab::NoiseKey SessionTab::currentNoiseKey() const
{
    NoiseKey key;
    key.light = m_light.id;
    key.dark = m_dark.id;
    key.darkFrame = m_darkPanel->frameMode() == FrameMode::Mean ? -1 : m_darkPanel->currentFrame();
    return key;
}

void SessionTab::scheduleCalibratedNoise()
{
    if (!m_calNoiseTimer->isActive())
        addPending(+1);  // counts as work in progress (isBusy) until it has run
    m_calNoiseTimer->start();
}

void SessionTab::startCalibratedNoise()
{
    if (!isAutoCalibration() || m_light.isNull())
        return;
    const Image2D dark = m_dark.shown;
    const ImageStack stack = m_light.stack;
    if (dark.isNull() || dark.width() != stack.cols() || dark.height() != stack.rows())
        return;
    const NoiseKey key = currentNoiseKey();
    const int frames = stack.frameCount();
    if (m_calNoiseValid && key == m_calNoiseKey) {
        if (m_calPanel->hasImage())
            m_calPanel->setNoise(m_calNoise, frames);
        return;
    }

    runInBackground<double>(
        [stack, dark] { return processing::calculateImageNoise(stack, dark); },
        [this, key, frames](double noise) {
            if (!(key == currentNoiseKey()))
                return;  // the light stack or the dark image has changed since
            m_calNoise = noise;
            m_calNoiseKey = key;
            m_calNoiseValid = true;
            if (isAutoCalibration() && m_calPanel->hasImage())
                m_calPanel->setNoise(noise, frames);
        });
}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------

QString SessionTab::defaultExportName(ImagePanel* panel, ExportKind kind) const
{
    auto frameTag = [](const ImagePanel* p) {
        return QStringLiteral("frame%1").arg(p->currentFrame() + 1, 3, 10, QLatin1Char('0'));
    };
    const bool stack = kind == ExportKind::StackMat;
    const QString ext = kind == ExportKind::ImagePng ? QStringLiteral(".png") : QStringLiteral(".mat");

    QString name;
    switch (panel->role()) {
    case PanelRole::Light:
        name = baseNameFor(m_light.paths)
               + (stack ? QStringLiteral("_light_all")
                  : m_lightPanel->frameMode() == FrameMode::Mean ? QStringLiteral("_light_mean")
                                                                 : QStringLiteral("_light_") + frameTag(m_lightPanel));
        break;
    case PanelRole::Dark:
        name = baseNameFor(m_dark.paths)
               + (stack ? QStringLiteral("_dark_all")
                  : m_darkPanel->frameMode() == FrameMode::Mean ? QStringLiteral("_dark_mean")
                                                                : QStringLiteral("_dark_") + frameTag(m_darkPanel));
        break;
    case PanelRole::Calibrated:
        if (!m_cal.isNull())
            name = baseNameFor(m_cal.paths)
                   + (stack ? QStringLiteral("_calibrated_all")
                      : m_cal.frames() > 1 ? QStringLiteral("_calibrated_") + frameTag(m_calPanel)
                                           : QStringLiteral("_calibrated"));
        else
            name = baseNameFor(m_light.paths)
                   + (stack ? QStringLiteral("_calibrated_all")
                      : m_lightPanel->frameMode() == FrameMode::Mean ? QStringLiteral("_calibrated_mean")
                                                                     : QStringLiteral("_calibrated_") + frameTag(m_lightPanel));
        break;
    }
    return name + ext;
}

void SessionTab::exportFrom(ImagePanel* panel, ExportKind kind)
{
    if (!panel->hasImage())
        return;

    QSettings settings;
    const QStringList& sourcePaths = panel->role() == PanelRole::Dark ? m_dark.paths
                                     : panel->role() == PanelRole::Calibrated && !m_cal.isNull() ? m_cal.paths
                                                                                                 : m_light.paths;
    const QString sourceDir = sourcePaths.isEmpty() ? QDir::homePath() : QFileInfo(sourcePaths.first()).absolutePath();
    const QString dir = settings.value(QStringLiteral("lastExportDirectory"), sourceDir).toString();
    const QString suggested = QDir(dir).filePath(defaultExportName(panel, kind));

    QString title;
    QString filter;
    if (kind == ExportKind::ImagePng) {
        title = tr("Save %1 image as PNG").arg(panel->title().toLower());
        filter = tr("PNG image (*.png)");
    } else {
        title = kind == ExportKind::StackMat ? tr("Save all %1 as .mat").arg(panel->title().toLower())
                                             : tr("Save %1 data as .mat").arg(panel->title().toLower());
        filter = tr("MATLAB data (*.mat)");
    }
    QString path = QFileDialog::getSaveFileName(this, title, suggested, filter);
    if (path.isEmpty())
        return;
    const QString wantedExt = kind == ExportKind::ImagePng ? QStringLiteral(".png") : QStringLiteral(".mat");
    if (!path.endsWith(wantedExt, Qt::CaseInsensitive))
        path += wantedExt;
    settings.setValue(QStringLiteral("lastExportDirectory"), QFileInfo(path).absolutePath());
    exportTo(panel, kind, path);
}

void SessionTab::exportTo(ImagePanel* panel, ExportKind kind, const QString& path)
{
    const QString nativePath = QDir::toNativeSeparators(path);
    auto finish = [this, nativePath, path](const QString& error) {
        if (error.isEmpty()) {
            emit statusMessage(tr("Saved %1").arg(nativePath));
        } else {
            QMessageBox::warning(this, tr("Could not save"), error);
            emit statusMessage(tr("Could not save %1").arg(nativePath));
        }
        emit exportFinished(path, error);
    };

    if (!panel->hasImage()) {
        finish(tr("There is no image to save."));
        return;
    }

    if (kind == ExportKind::ImagePng) {
        const QImage img = panel->displayedImage();
        finish(img.save(path, "PNG") ? QString() : tr("Could not write %1.").arg(nativePath));
        return;
    }

    // Everything the worker needs, captured by value (implicitly shared, so cheap).
    const PanelRole role = panel->role();
    const Image2D image = panel->image();
    const processing::MeanStd ms = panel->meanStd();
    const double noise = panel->noise();
    const bool lightMean = m_lightPanel->frameMode() == FrameMode::Mean;
    const bool darkMean = m_darkPanel->frameMode() == FrameMode::Mean;
    const int lightIndex = m_lightPanel->currentFrame() + 1;
    const int darkIndex = m_darkPanel->currentFrame() + 1;
    const int calIndex = m_calPanel->currentFrame() + 1;
    const bool manualCal = !m_cal.isNull();
    const ImageStack light = m_light.stack;
    const ImageStack dark = m_dark.stack;
    const ImageStack cal = m_cal.stack;
    auto classOr = [](const QString& c) { return c.isEmpty() ? QStringLiteral("double") : c; };
    const QString lightClass = classOr(m_light.matlabClass);
    const QString darkClass = classOr(m_dark.matlabClass);
    const QString calClass = classOr(m_cal.matlabClass);
    const Image2D darkImage = m_dark.shown;
    const int lightFrames = m_light.frames();
    const int darkFrames = m_dark.frames();
    const int calFrames = m_cal.frames();
    const QString lightFile = fileList(m_light.paths);
    const QString darkFile = fileList(m_dark.paths);
    const QString calFile = fileList(m_cal.paths);
    const QString frameFormula = calibrationFormula(false);
    const QString stackFormula = calibrationFormula(true);

    emit statusMessage(tr("Saving %1 …").arg(nativePath));
    runInBackground<QString>(
        [=]() -> QString {
            MatFileWriter w;
            bool ok = true;
            auto addStats = [&] {
                return w.addScalar(QStringLiteral("img_mean"), ms.mean) && w.addScalar(QStringLiteral("img_std"), ms.std);
            };
            switch (role) {
            case PanelRole::Light:
                if (kind == ExportKind::StackMat)
                    ok = w.addStack(QStringLiteral("light"), light, lightClass);
                else if (lightMean)
                    ok = w.addImage(QStringLiteral("light_mean"), image)
                         && w.addScalar(QStringLiteral("light_frame_count"), lightFrames) && addStats();
                else
                    ok = w.addImage(QStringLiteral("light_frame"), image)
                         && w.addScalar(QStringLiteral("frame_index"), lightIndex) && addStats();
                ok = ok && w.addScalar(QStringLiteral("img_noise"), noise)
                        && w.addString(QStringLiteral("source_file"), lightFile);
                break;
            case PanelRole::Dark:
                if (kind == ExportKind::StackMat)
                    ok = w.addStack(QStringLiteral("dark"), dark, darkClass);
                else if (darkMean)
                    ok = w.addImage(QStringLiteral("dark_mean"), image) && addStats();
                else
                    ok = w.addImage(QStringLiteral("dark_frame"), image)
                         && w.addScalar(QStringLiteral("frame_index"), darkIndex) && addStats();
                ok = ok && w.addScalar(QStringLiteral("img_noise"), noise)
                        && w.addScalar(QStringLiteral("dark_frame_count"), darkFrames)
                        && w.addString(QStringLiteral("source_file"), darkFile);
                break;
            case PanelRole::Calibrated:
                if (manualCal) {
                    // A calibrated image loaded from a file.
                    if (kind == ExportKind::StackMat)
                        ok = w.addStack(QStringLiteral("calibrated"), cal, calClass);
                    else
                        ok = w.addImage(QStringLiteral("calibrated_frame"), image)
                             && (calFrames < 2 || w.addScalar(QStringLiteral("frame_index"), calIndex)) && addStats();
                    ok = ok && w.addScalar(QStringLiteral("img_noise"), noise)
                            && w.addString(QStringLiteral("source_file"), calFile);
                    break;
                }
                if (kind == ExportKind::StackMat) {
                    // light(:, :, k) - dark image, for every k.
                    const std::vector<double> offset = processing::toColumnMajor(darkImage);
                    ok = w.addDoubleStack(QStringLiteral("calibrated"), light.rows(), light.cols(), light.frameCount(),
                                          [&](int z, double* out) {
                                              processing::frameMinusOffset(light, z, offset.data(), out);
                                          })
                         && w.addString(QStringLiteral("calibration"), stackFormula);
                } else {
                    ok = w.addImage(QStringLiteral("calibrated_frame"), image)
                         && w.addString(QStringLiteral("calibration"), frameFormula)
                         && (lightMean ? w.addScalar(QStringLiteral("light_frame_count"), lightFrames)
                                       : w.addScalar(QStringLiteral("frame_index"), lightIndex))
                         && addStats();
                }
                ok = ok && (darkMean || w.addScalar(QStringLiteral("dark_frame_index"), darkIndex))
                        && w.addScalar(QStringLiteral("img_noise"), noise)
                        && w.addScalar(QStringLiteral("dark_frame_count"), darkFrames)
                        && w.addString(QStringLiteral("light_file"), lightFile)
                        && w.addString(QStringLiteral("dark_file"), darkFile);
                break;
            }
            if (!ok || !w.save(path))
                return w.errorString();
            return QString();
        },
        finish);
}

} // namespace iat
