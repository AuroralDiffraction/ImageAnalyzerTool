#include "ui/SessionTab.h"

#include "core/Processing.h"
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
#include <QtConcurrent/QtConcurrentRun>

#include <cmath>
#include <utility>
#include <vector>

namespace iat {

namespace {

QString describe(const QString& path, const MatVariable& v)
{
    return QStringLiteral("%1 · %2 · %3 %4")
        .arg(QFileInfo(path).fileName(), v.name, v.sizeString(), v.matlabClass);
}

QString formatNoise(double v) { return QString::number(v, 'g', 7); }

struct DarkResult {
    Image2D mean;
    double noise = std::numeric_limits<double>::quiet_NaN();
};

} // namespace

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

    connect(m_lightPanel, &ImagePanel::fileDropped, this, &SessionTab::loadLight);
    connect(m_darkPanel, &ImagePanel::fileDropped, this, &SessionTab::loadDark);
    connect(m_lightPanel, &ImagePanel::loadRequested, this, &SessionTab::openLightDialog);
    connect(m_darkPanel, &ImagePanel::loadRequested, this, &SessionTab::openDarkDialog);
    connect(m_lightPanel, &ImagePanel::currentFrameChanged, this, [this](int index) {
        // The noise values describe whole stacks, so they stay as they are.
        showLightFrame(index, false);
        updateCalibrated(false);
    });
    for (ImagePanel* p : {m_lightPanel, m_darkPanel, m_calPanel}) {
        connect(p, &ImagePanel::exportRequested, this, [this, p](ExportKind kind) { exportFrom(p, kind); });
        connect(p, &ImagePanel::message, this, &SessionTab::statusMessage);
    }
    linkViews();

    updateCalibrated(true);
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

void SessionTab::loadLight(const QString& path) { startRead(Target::Light, path); }
void SessionTab::loadDark(const QString& path) { startRead(Target::Dark, path); }

QString SessionTab::openDialog(const QString& title)
{
    QSettings settings;
    const QString dir = settings.value(QStringLiteral("lastDirectory"), QDir::homePath()).toString();
    const QString path = QFileDialog::getOpenFileName(this, title, dir,
                                                      tr("MATLAB data (*.mat);;All files (*)"));
    if (!path.isEmpty())
        settings.setValue(QStringLiteral("lastDirectory"), QFileInfo(path).absolutePath());
    return path;
}

void SessionTab::openLightDialog()
{
    const QString path = openDialog(tr("Open light frames"));
    if (!path.isEmpty())
        loadLight(path);
}

void SessionTab::openDarkDialog()
{
    const QString path = openDialog(tr("Open dark frames"));
    if (!path.isEmpty())
        loadDark(path);
}

void SessionTab::addPending(int delta)
{
    const bool wasBusy = isBusy();
    m_pending += delta;
    if (wasBusy != isBusy())
        emit busyChanged(isBusy());
}

void SessionTab::startRead(Target target, const QString& path)
{
    const quint64 gen = ++generation(target);
    ImagePanel* panel = target == Target::Light ? m_lightPanel : m_darkPanel;
    panel->showBusy(tr("Reading %1 …").arg(QFileInfo(path).fileName()));
    emit statusMessage(tr("Reading %1 …").arg(QDir::toNativeSeparators(path)));

    runInBackground<MatFileContents>(
        [path] { return MatFileReader::read(path); },
        [this, target, gen, path](const MatFileContents& contents) {
            if (gen == generation(target))
                onFileRead(target, path, contents);
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
        tr("%1 contains several arrays.\nWhich one holds the %2 frames?")
            .arg(QFileInfo(path).fileName(), target == Target::Light ? tr("light") : tr("dark")),
        items, 0, false, &ok);
    if (!ok)
        return std::nullopt;
    return stacks.at(int(items.indexOf(item)));
}

void SessionTab::onFileRead(Target target, const QString& path, const MatFileContents& contents)
{
    if (!contents.ok()) {
        QMessageBox::warning(this, tr("Could not open file"),
                             QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), contents.error));
        emit statusMessage(tr("Could not open %1").arg(QFileInfo(path).fileName()));
        restorePanel(target);
        return;
    }

    const std::optional<MatVariable> chosen = chooseVariable(target, path, contents);
    if (!chosen) {
        restorePanel(target);
        return;
    }

    if (target == Target::Light)
        setLightStack(path, *chosen);
    else
        startDarkMean(path, *chosen);
}

void SessionTab::setLightStack(const QString& path, const MatVariable& variable)
{
    m_lightStack = variable.data;
    m_lightPath = path;
    m_lightClass = variable.matlabClass;
    m_lightNoise = std::numeric_limits<double>::quiet_NaN();
    m_lightInfo = describe(path, variable);
    m_lightPanel->setSourceInfo(m_lightInfo, QDir::toNativeSeparators(path));
    m_lightPanel->setFrameCount(m_lightStack.frameCount());  // selects the first frame
    showLightFrame(0, true);
    startLightNoise();
    updateCalibrated(true);
    startCalibratedNoise();
    emit statusMessage(tr("Loaded light frames: %1").arg(m_lightInfo));
}

void SessionTab::startLightNoise()
{
    if (m_lightStack.isNull())
        return;
    const quint64 gen = m_lightGeneration;
    const ImageStack stack = m_lightStack;
    const int frames = stack.frameCount();
    m_lightPanel->setNoisePending();

    runInBackground<double>(
        [stack] { return processing::calculateImageNoise(stack); },
        [this, gen, frames](double noise) {
            if (gen != m_lightGeneration)
                return;
            m_lightNoise = noise;
            m_lightPanel->setNoise(noise, frames);
            emit statusMessage(tr("Calculated noise of light frames [DN]: %1").arg(formatNoise(noise)));
        });
}

void SessionTab::startDarkMean(const QString& path, const MatVariable& variable)
{
    const quint64 gen = generation(Target::Dark);
    const ImageStack stack = variable.data;
    const int frames = stack.frameCount();
    const QString info = describe(path, variable);
    m_darkPanel->showBusy(tr("Averaging %1 dark frames …").arg(frames));

    runInBackground<DarkResult>(
        [stack] {
            DarkResult r;
            r.mean = processing::meanOverFrames(stack);
            r.noise = processing::calculateImageNoise(stack);
            return r;
        },
        [this, gen, path, info, frames](const DarkResult& r) {
            if (gen != generation(Target::Dark))
                return;
            m_darkMean = r.mean;
            m_darkNoise = r.noise;
            m_darkPath = path;
            m_darkFrameCount = frames;
            m_darkPanel->setSourceInfo(tr("%1 · mean of %2 frames").arg(info).arg(frames),
                                       QDir::toNativeSeparators(path));
            m_darkPanel->setImage(m_darkMean, true);
            m_darkPanel->setNoise(m_darkNoise, frames);
            updateCalibrated(true);
            startCalibratedNoise();
            emit statusMessage(tr("Loaded dark frames: %1 · calculated noise of dark frames [DN]: %2")
                                   .arg(info, formatNoise(m_darkNoise)));
        });
}

void SessionTab::startCalibratedNoise()
{
    if (m_lightStack.isNull() || m_darkMean.isNull() || !m_lightFrame.sameSize(m_darkMean))
        return;
    const quint64 lightGen = m_lightGeneration;
    const quint64 darkGen = m_darkGeneration;
    const ImageStack stack = m_lightStack;
    const Image2D dark = m_darkMean;
    const int frames = stack.frameCount();
    m_calPanel->setNoisePending();

    runInBackground<double>(
        [stack, dark] { return processing::calculateImageNoise(stack, dark); },
        [this, lightGen, darkGen, frames](double noise) {
            if (lightGen == m_lightGeneration && darkGen == m_darkGeneration && m_calPanel->hasImage())
                m_calPanel->setNoise(noise, frames);
        });
}

void SessionTab::restorePanel(Target target)
{
    if (target == Target::Light) {
        if (m_lightStack.isNull()) {
            m_lightPanel->showPlaceholder(QString());
            return;
        }
        m_lightPanel->setImage(m_lightFrame, false);
        if (std::isnan(m_lightNoise))
            startLightNoise();  // the earlier result was dropped by the cancelled load
        else
            m_lightPanel->setNoise(m_lightNoise, m_lightStack.frameCount());
    } else {
        if (m_darkMean.isNull()) {
            m_darkPanel->showPlaceholder(QString());
            return;
        }
        m_darkPanel->setImage(m_darkMean, false);
        m_darkPanel->setNoise(m_darkNoise, m_darkFrameCount);
    }
}

void SessionTab::showLightFrame(int index, bool resetContrast)
{
    m_lightFrame = m_lightStack.frame(index);
    m_lightPanel->setImage(m_lightFrame, resetContrast);
}

void SessionTab::updateCalibrated(bool resetContrast)
{
    const bool haveLight = !m_lightFrame.isNull();
    const bool haveDark = !m_darkMean.isNull();

    if (!haveLight || !haveDark) {
        QString msg;
        if (!haveLight && !haveDark)
            msg = QString();  // default text
        else if (!haveLight)
            msg = tr("Dark frame ready.\n\nNow load light frames to compute\nlight frame − mean dark frame.");
        else
            msg = tr("Light frames ready.\n\nNow load dark frames to compute\nlight frame − mean dark frame.");
        m_calPanel->showPlaceholder(msg);
        m_calPanel->setSourceInfo(tr("Light frame − mean dark frame"));
        return;
    }

    if (!m_lightFrame.sameSize(m_darkMean)) {
        m_calPanel->showPlaceholder(
            tr("The sizes do not match:\n\nlight frames  %1 × %2\ndark frame    %3 × %4\n\n"
               "Load files with the same X × Y size.")
                .arg(m_lightFrame.height()).arg(m_lightFrame.width())
                .arg(m_darkMean.height()).arg(m_darkMean.width()));
        m_calPanel->setSourceInfo(tr("Light frame − mean dark frame"));
        return;
    }

    const Image2D calibrated = processing::subtract(m_lightFrame, m_darkMean);
    const int frame = m_lightPanel->currentFrame() + 1;
    m_calPanel->setSourceInfo(tr("Light frame %1 of %2 − mean of %3 dark frames")
                                  .arg(frame)
                                  .arg(m_lightStack.frameCount())
                                  .arg(m_darkFrameCount));
    m_calPanel->setImage(calibrated, resetContrast || !m_calPanel->hasImage());
}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------

QString SessionTab::defaultExportName(ImagePanel* panel, ExportKind kind) const
{
    const QString lightBase = QFileInfo(m_lightPath).completeBaseName();
    const QString darkBase = QFileInfo(m_darkPath).completeBaseName();
    const QString frame = QStringLiteral("frame%1").arg(m_lightPanel->currentFrame() + 1, 3, 10, QLatin1Char('0'));
    const QString ext = kind == ExportKind::ImagePng ? QStringLiteral(".png") : QStringLiteral(".mat");

    QString name;
    switch (panel->role()) {
    case PanelRole::Light:
        name = lightBase + (kind == ExportKind::StackMat ? QStringLiteral("_light_all") : QStringLiteral("_light_") + frame);
        break;
    case PanelRole::Dark:
        name = darkBase + QStringLiteral("_dark_mean");
        break;
    case PanelRole::Calibrated:
        name = lightBase + (kind == ExportKind::StackMat ? QStringLiteral("_calibrated_all")
                                                         : QStringLiteral("_calibrated_") + frame);
        break;
    }
    return name + ext;
}

void SessionTab::exportFrom(ImagePanel* panel, ExportKind kind)
{
    if (!panel->hasImage())
        return;

    QSettings settings;
    const QString sourceDir = QFileInfo(panel->role() == PanelRole::Dark ? m_darkPath : m_lightPath).absolutePath();
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
    const int frameIndex = m_lightPanel->currentFrame() + 1;
    const ImageStack light = m_lightStack;
    const QString lightClass = m_lightClass.isEmpty() ? QStringLiteral("double") : m_lightClass;
    const Image2D darkMean = m_darkMean;
    const int darkFrames = m_darkFrameCount;
    const QString lightFile = QDir::toNativeSeparators(m_lightPath);
    const QString darkFile = QDir::toNativeSeparators(m_darkPath);

    emit statusMessage(tr("Saving %1 …").arg(nativePath));
    runInBackground<QString>(
        [=]() -> QString {
            MatFileWriter w;
            bool ok = true;
            switch (role) {
            case PanelRole::Light:
                if (kind == ExportKind::StackMat) {
                    ok = w.addStack(QStringLiteral("light"), light, lightClass);
                } else {
                    ok = w.addImage(QStringLiteral("light_frame"), image)
                         && w.addScalar(QStringLiteral("frame_index"), frameIndex)
                         && w.addScalar(QStringLiteral("img_mean"), ms.mean)
                         && w.addScalar(QStringLiteral("img_std"), ms.std);
                }
                ok = ok && w.addScalar(QStringLiteral("img_noise"), noise)
                        && w.addString(QStringLiteral("source_file"), lightFile);
                break;
            case PanelRole::Dark:
                ok = w.addImage(QStringLiteral("dark_mean"), image)
                     && w.addScalar(QStringLiteral("img_mean"), ms.mean)
                     && w.addScalar(QStringLiteral("img_std"), ms.std)
                     && w.addScalar(QStringLiteral("img_noise"), noise)
                     && w.addScalar(QStringLiteral("dark_frame_count"), darkFrames)
                     && w.addString(QStringLiteral("source_file"), darkFile);
                break;
            case PanelRole::Calibrated:
                if (kind == ExportKind::StackMat) {
                    const std::vector<double> dark = processing::toColumnMajor(darkMean);
                    ok = w.addDoubleStack(QStringLiteral("calibrated"), light.rows(), light.cols(), light.frameCount(),
                                          [&](int z, double* out) {
                                              processing::frameMinusOffset(light, z, dark.data(), out);
                                          });
                } else {
                    ok = w.addImage(QStringLiteral("calibrated_frame"), image)
                         && w.addScalar(QStringLiteral("frame_index"), frameIndex)
                         && w.addScalar(QStringLiteral("img_mean"), ms.mean)
                         && w.addScalar(QStringLiteral("img_std"), ms.std);
                }
                ok = ok && w.addScalar(QStringLiteral("img_noise"), noise)
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
