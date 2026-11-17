// UI smoke test: opens the main window and checks single image mode (the
// default) in the first tab, then switches the tab to three panels, loads
// dark frames, waits for loading to finish and checks the results, the frame
// buttons, linked zoom/pan, the clipboard copies and the exports. With the
// small test files it also checks the Single frame / Mean modes, loading
// several image files, a calibrated image loaded from a file, and the image
// processing pipeline of a panel (the Processing… button). These
// checks run with the English texts of the source code; at the end the
// Chinese translation (the app's default language) is installed and checked.
//
//   ui_smoke                                  uses the small test files
//   ui_smoke light.mat dark.mat [shot.png] [frame]
//
// Exported files go to a temporary folder, or to IAT_SMOKE_EXPORT_DIR if set.
// Run headless with QT_QPA_PLATFORM=offscreen.

#include "core/Processing.h"
#include "io/ImageFileReader.h"
#include "io/MatFileReader.h"
#include "ui/AboutDialog.h"
#include "ui/ImagePanel.h"
#include "ui/ImageView.h"
#include "ui/Language.h"
#include "ui/MainWindow.h"
#include "ui/ModeSwitch.h"
#include "ui/PipelineDialog.h"
#include "ui/SessionTab.h"
#include "ui/SettingsDialog.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QMenuBar>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSlider>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolButton>
#include <QUrl>
#include <QWheelEvent>

#include <cmath>
#include <cstdio>

using namespace iat;

static bool g_ok = true;

static void check(bool condition, const char* what)
{
    std::printf("  %-58s %s\n", what, condition ? "ok" : "FAILED");
    g_ok = g_ok && condition;
}

static bool waitUntilIdle(const MainWindow& w, int timeoutMs)
{
    QElapsedTimer t;
    t.start();
    while (w.isBusy() && t.elapsed() < timeoutMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    for (int i = 0; i < 10; ++i)
        QCoreApplication::processEvents();
    return !w.isBusy();
}

static void report(const char* name, const ImagePanel* p)
{
    std::printf("%-11s image=%s  %dx%d  mean=%.9f  std=%.9f  noise=%.9f  display=[%g, %g]\n",
                name, p->hasImage() ? "yes" : "no ", p->image().height(), p->image().width(),
                p->meanStd().mean, p->meanStd().std, p->noise(), p->displayLow(), p->displayHigh());
}

static bool near(double a, double b, double tol = 1e-9) { return std::abs(a - b) <= tol * std::max(1.0, std::abs(b)); }

static MatVariable variable(const MatFileContents& c, const char* name)
{
    for (const MatVariable& v : c.variables)
        if (v.name == QLatin1String(name))
            return v;
    return {};
}

static double scalar(const MatFileContents& c, const char* name)
{
    const MatVariable v = variable(c, name);
    return v.supported ? v.data.frame(0).at(0, 0) : std::nan("");
}

// Simulates dragging files onto a panel and dropping them. Returns whether
// the panel accepted the drag.
static bool dropFiles(ImagePanel* target, const QStringList& paths, bool drop = true)
{
    QMimeData mime;
    QList<QUrl> urls;
    for (const QString& p : paths)
        urls << QUrl::fromLocalFile(p);
    mime.setUrls(urls);
    const QPoint c = target->rect().center();
    QDragEnterEvent enter(c, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(target, &enter);
    const bool accepted = enter.isAccepted();
    if (accepted && drop) {
        QDropEvent dropEvent(QPointF(c), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(target, &dropEvent);
    }
    return accepted;
}

// Length of a 1 × N char variable (its text is not loaded by the reader).
static qint64 textLength(const MatFileContents& c, const char* name)
{
    const MatVariable v = variable(c, name);
    return v.matlabClass == QLatin1String("char") && v.dims.size() == 2 ? v.dims.at(1) : -1;
}

// Clicks the single image (left) or three panels (right) side of the switch.
static void clickSwitch(ModeSwitch* sw, LayoutMode side)
{
    const QPointF pos(sw->width() * (side == LayoutMode::Single ? 0.25 : 0.75), sw->height() / 2.0);
    QMouseEvent press(QEvent::MouseButtonPress, pos, sw->mapToGlobal(pos), Qt::LeftButton, Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(sw, &press);
}

static bool sameImage(const Image2D& a, const Image2D& b)
{
    if (!a.sameSize(b))
        return false;
    for (qsizetype i = 0; i < a.pixelCount(); ++i)
        if (a.constData()[i] != b.constData()[i])
            return false;
    return true;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ImageAnalyzerToolSmokeTest"));
    QIcon icon;  // as in main.cpp
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addFile(QStringLiteral(":/icons/app_%1.png").arg(size), QSize(size, size));
    QApplication::setWindowIcon(icon);

    const QStringList args = app.arguments();
    const QString fixtureLight = QStringLiteral(IAT_TEST_DATA_DIR "/light_small.mat");
    const QString fixtureDark = QStringLiteral(IAT_TEST_DATA_DIR "/dark_small.mat");
    const QString light = args.size() < 3 ? fixtureLight : args.at(1);
    const QString dark = args.size() < 3 ? fixtureDark : args.at(2);
    // The checks against known values run whenever the test files are used.
    const bool useFixtures = QFileInfo(light) == QFileInfo(fixtureLight) && QFileInfo(dark) == QFileInfo(fixtureDark);
    const QString shot = args.size() > 3 ? args.at(3) : QString();
    const int frame = args.size() > 4 ? args.at(4).toInt() : 1;

    QTemporaryDir tempDir;
    const QString exportDir = qEnvironmentVariable("IAT_SMOKE_EXPORT_DIR", tempDir.path());
    QDir().mkpath(exportDir);

    MainWindow w;
    w.resize(1600, 1000);
    w.show();

    SessionTab* tab = w.currentTab();
    ImagePanel* lp = tab->lightPanel();
    ImagePanel* dp = tab->darkPanel();
    ImagePanel* cp = tab->calibratedPanel();
    ModeSwitch* sw = w.modeSwitch();
    auto out = [&](const char* name) { return QDir(exportDir).filePath(QLatin1String(name)); };

    // ---- single image mode (the default)
    std::printf("single image mode\n");
    QCoreApplication::processEvents();
    check(tab->layoutMode() == LayoutMode::Single && sw && sw->isVisible() && sw->mode() == LayoutMode::Single,
          "the app starts in single image mode");
    check(lp->isVisible() && !dp->isVisible() && !cp->isVisible() && lp->title() == QLatin1String("Image"),
          "one panel on show, titled Image");
    check(lp->width() > tab->width() * 0.9, "the panel fills the tab");
    tab->loadImage(light);
    if (!waitUntilIdle(w, 120000)) {
        std::printf("FAIL: loading did not finish\n");
        return 1;
    }
    check(lp->hasImage() && !cp->hasImage() && lp->frameCount() > 0, "the image loads into the panel, with a frame selector");
    if (useFixtures)
        check(near(lp->meanStd().mean, 1285.0) && near(lp->meanStd().std, 172.312700247738)
                  && near(lp->noise(), 1.118033988750), "Mean/Std/Noise match NumPy");
    {
        const double autoLow = lp->displayLow();
        const double autoHigh = lp->displayHigh();
        lp->fullRangeContrast();
        check(lp->displayLow() == lp->stats().min && lp->displayHigh() == lp->stats().max, "Full range: min to max");
        lp->autoContrast();
        check(lp->displayLow() == autoLow && lp->displayHigh() == autoHigh, "Auto: back to the percentile range");
    }
    tab->exportTo(lp, ExportKind::FrameMat, out("single_frame.mat"));
    tab->exportTo(lp, ExportKind::StackMat, out("single_all.mat"));
    check(waitUntilIdle(w, 300000), "exports finished");
    {
        const MatFileContents sf = MatFileReader::read(out("single_frame.mat"));
        check(sf.ok() && sameImage(variable(sf, "frame").data.frame(0), lp->image())
                  && near(scalar(sf, "img_mean"), lp->meanStd().mean) && near(scalar(sf, "img_noise"), lp->noise())
                  && scalar(sf, "frame_index") == 1.0 && variable(sf, "light_frame").name.isEmpty(),
              "single_frame.mat: frame + Mean/Std/Noise + frame index");
        const MatFileContents sa = MatFileReader::read(out("single_all.mat"));
        const MatFileContents in = MatFileReader::read(light);
        check(sa.ok() && in.ok() && variable(sa, "frames").data.rawData() == in.imageStacks().first().data.rawData(),
              "single_all.mat: identical to the input stack");
    }
    if (!shot.isEmpty()) {
        QString singleShot = shot;
        singleShot.replace(QLatin1String(".png"), QLatin1String("_single.png"));
        w.statusBar()->clearMessage();
        if (w.grab().save(singleShot))
            std::printf("single image mode: %s\n", qPrintable(singleShot));
    }

    {
        // The cursor over the image is a crosshair, also after a drag.
        QWidget* vp = lp->view()->viewport();
        const bool before = vp->cursor().shape() == Qt::CrossCursor;
        const QPointF c = QPointF(vp->rect().center());
        QMouseEvent press(QEvent::MouseButtonPress, c, vp->mapToGlobal(c), Qt::LeftButton, Qt::LeftButton,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(vp, &press);
        const bool dragging = vp->cursor().shape() == Qt::ClosedHandCursor;
        QMouseEvent release(QEvent::MouseButtonRelease, c, vp->mapToGlobal(c), Qt::LeftButton, Qt::NoButton,
                            Qt::NoModifier);
        QCoreApplication::sendEvent(vp, &release);
        check(before && dragging && vp->cursor().shape() == Qt::CrossCursor,
              "crosshair cursor over the image (closed hand while dragging)");
    }

    // Zoom in, then switch to three panels with the switch next to the tabs.
    for (int i = 0; i < 3; ++i) {
        const QPointF c = QPointF(lp->view()->viewport()->rect().center());
        QWheelEvent wheel(c, lp->view()->viewport()->mapToGlobal(c), QPoint(), QPoint(0, 120), Qt::NoButton,
                          Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(lp->view()->viewport(), &wheel);
    }
    const Image2D singleImage = lp->image();
    const double singleZoom = lp->view()->viewState().zoom;
    clickSwitch(sw, LayoutMode::Triple);
    waitUntilIdle(w, 5000);
    check(tab->layoutMode() == LayoutMode::Triple && sw->mode() == LayoutMode::Triple && dp->isVisible()
              && cp->isVisible() && lp->title() == QLatin1String("Light frames"),
          "the switch shows the three panels");
    check(sameImage(lp->image(), singleImage), "the image is kept, as the light frames");
    check(std::abs(lp->width() - dp->width()) <= 2 && std::abs(dp->width() - cp->width()) <= 2,
          "the three panels have the same width");

    tab->loadDark(dark);
    if (!waitUntilIdle(w, 120000)) {
        std::printf("FAIL: loading did not finish\n");
        return 1;
    }
    check(!lp->view()->viewState().fit && std::abs(lp->view()->viewState().zoom - singleZoom) < 1e-9
              && std::abs(dp->view()->viewState().zoom - singleZoom) < 1e-9
              && std::abs(cp->view()->viewState().zoom - singleZoom) < 1e-9,
          "the zoom from single image mode carries over to all three views");
    lp->view()->fitToWindowByUser();
    QCoreApplication::processEvents();

    // Back to a single image and forth: the hidden calibrated panel catches up.
    clickSwitch(sw, LayoutMode::Single);
    waitUntilIdle(w, 5000);
    check(tab->layoutMode() == LayoutMode::Single && !dp->isVisible() && !cp->isVisible()
              && lp->title() == QLatin1String("Image") && sameImage(lp->image(), singleImage),
          "back to the single image");
    lp->stepFrame(+1);
    waitUntilIdle(w, 5000);
    tab->setLayoutMode(LayoutMode::Triple);
    waitUntilIdle(w, 5000);
    check(sw->mode() == LayoutMode::Triple && cp->hasImage()
              && sameImage(cp->image(), processing::subtract(lp->image(), dp->image())),
          "three panels again: calibrated = the new light frame - dark");
    lp->setCurrentFrame(0);
    waitUntilIdle(w, 5000);
    if (frame > 1) {
        lp->setCurrentFrame(frame - 1);
        waitUntilIdle(w, 1000);
    }

    report("light", lp);
    report("dark", dp);
    report("calibrated", cp);

    std::printf("results\n");
    check(lp->hasImage() && dp->hasImage() && cp->hasImage(), "all three panels show an image");
    check(near(cp->meanStd().mean, lp->meanStd().mean - dp->meanStd().mean, 1e-6), "calibrated mean = light mean - dark mean");
    check(near(cp->noise(), lp->noise()), "calibrated noise = light noise");
    if (useFixtures) {
        check(near(lp->meanStd().mean, 1285.0) && near(lp->meanStd().std, 172.312700247738)
                  && near(lp->noise(), 1.118033988750), "light Mean/Std/Noise match NumPy");
        check(near(dp->meanStd().mean, 14.25) && near(dp->meanStd().std, 2.056493779876)
                  && near(dp->noise(), 8.164965809277), "dark Mean/Std/Noise match NumPy");
        check(near(cp->meanStd().mean, 1270.75) && near(cp->meanStd().std, 170.470122015560),
              "calibrated Mean/Std match NumPy");
    }

    // ---- frame buttons
    std::printf("frame selection\n");
    const int startFrame = lp->currentFrame();
    lp->setCurrentFrame(0);
    waitUntilIdle(w, 1000);
    lp->stepFrame(-1);
    check(lp->currentFrame() == 0, "previous at frame 1 stays at frame 1");
    lp->stepFrame(+1);
    waitUntilIdle(w, 1000);
    check(lp->currentFrame() == 1, "next goes to frame 2");
    if (useFixtures)
        check(near(cp->meanStd().mean, 1271.75) && near(cp->noise(), 1.118033988750),
              "calibrated follows (mean 1271.75), noise unchanged");
    const int lightFrames = MatFileReader::read(light).imageStacks().first().data.frameCount();
    lp->stepFrame(+1000);
    waitUntilIdle(w, 1000);
    check(lp->currentFrame() == lightFrames - 1, "next stops at the last frame");
    lp->setCurrentFrame(startFrame);
    waitUntilIdle(w, 1000);

    // ---- linked zoom and pan
    std::printf("linked zoom/pan\n");
    QCoreApplication::processEvents();
    ImageView* lv = lp->view();
    ImageView* dv = dp->view();
    ImageView* cv = cp->view();
    const QPointF centre = QPointF(lv->viewport()->rect().center());
    const double fitZoom = lv->viewState().zoom;
    for (int i = 0; i < 4; ++i) {
        QWheelEvent wheel(centre, lv->viewport()->mapToGlobal(centre), QPoint(), QPoint(0, 120), Qt::NoButton,
                          Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(lv->viewport(), &wheel);
    }
    QCoreApplication::processEvents();
    const ViewState ls = lv->viewState();
    check(!ls.fit && ls.zoom > fitZoom, "wheel zooms the light view");
    check(std::abs(dv->viewState().zoom - ls.zoom) < 1e-9 && std::abs(cv->viewState().zoom - ls.zoom) < 1e-9,
          "dark and calibrated views have the same zoom");
    lv->horizontalScrollBar()->setValue(lv->horizontalScrollBar()->value() + 40);
    lv->verticalScrollBar()->setValue(lv->verticalScrollBar()->value() + 25);
    QCoreApplication::processEvents();
    const QPointF lc = lv->viewState().center;
    const double tol = 1.0 / lv->viewState().zoom + 1e-6;  // one screen pixel
    check(std::abs(dv->viewState().center.x() - lc.x()) <= tol && std::abs(dv->viewState().center.y() - lc.y()) <= tol
              && std::abs(cv->viewState().center.x() - lc.x()) <= tol && std::abs(cv->viewState().center.y() - lc.y()) <= tol,
          "panning the light view pans the others to the same spot");
    cv->fitToWindowByUser();
    QCoreApplication::processEvents();
    check(lv->viewState().fit && dv->viewState().fit, "fit in one view fits all three");

    // ---- clipboard
    std::printf("clipboard\n");
    lp->copyImageToClipboard();
    const QImage clip = QApplication::clipboard()->image();
    check(clip.size() == QSize(lp->image().width(), lp->image().height()), "Copy image: full-resolution image");
    cp->copyReportToClipboard();
    const QMimeData* mime = QApplication::clipboard()->mimeData();
    const QImage figure = QApplication::clipboard()->image();
    check(!figure.isNull() && figure.size() == cp->renderReport().size(), "Copy with statistics: figure on the clipboard");
    check(mime->hasImage() && !mime->hasText() && !mime->hasHtml(), "Copy with statistics: image only (always pastes as a figure)");

    // ---- exports
    std::printf("exports (to %s)\n", qPrintable(QDir::toNativeSeparators(exportDir)));
    tab->exportTo(lp, ExportKind::FrameMat, out("light_frame.mat"));
    tab->exportTo(lp, ExportKind::StackMat, out("light_all.mat"));
    tab->exportTo(dp, ExportKind::FrameMat, out("dark_mean.mat"));
    tab->exportTo(cp, ExportKind::FrameMat, out("calibrated_frame.mat"));
    tab->exportTo(cp, ExportKind::StackMat, out("calibrated_all.mat"));
    tab->exportTo(lp, ExportKind::ImagePng, out("light.png"));
    tab->exportTo(cp, ExportKind::ImagePng, out("calibrated.png"));
    check(waitUntilIdle(w, 300000), "exports finished");

    const MatFileContents lf = MatFileReader::read(out("light_frame.mat"));
    check(lf.ok() && sameImage(variable(lf, "light_frame").data.frame(0), lp->image())
              && near(scalar(lf, "img_mean"), lp->meanStd().mean) && near(scalar(lf, "img_std"), lp->meanStd().std)
              && near(scalar(lf, "img_noise"), lp->noise()) && scalar(lf, "frame_index") == lp->currentFrame() + 1,
          "light_frame.mat: frame + Mean/Std/Noise + frame index");
    const MatFileContents la = MatFileReader::read(out("light_all.mat"));
    const MatFileContents src = MatFileReader::read(light);
    check(la.ok() && src.ok() && variable(la, "light").data.rawData() == src.imageStacks().first().data.rawData()
              && variable(la, "light").matlabClass == src.imageStacks().first().matlabClass,
          "light_all.mat: identical to the input stack");
    const MatFileContents dm = MatFileReader::read(out("dark_mean.mat"));
    check(dm.ok() && sameImage(variable(dm, "dark_mean").data.frame(0), dp->image())
              && near(scalar(dm, "img_noise"), dp->noise()),
          "dark_mean.mat: mean dark frame + noise");
    const MatFileContents cf = MatFileReader::read(out("calibrated_frame.mat"));
    check(cf.ok() && sameImage(variable(cf, "calibrated_frame").data.frame(0), cp->image()),
          "calibrated_frame.mat: calibrated frame");
    const MatFileContents ca = MatFileReader::read(out("calibrated_all.mat"));
    const MatVariable cal = variable(ca, "calibrated");
    const ImageStack lightStack = src.imageStacks().first().data;
    bool allFrames = ca.ok() && cal.data.frameCount() == lightStack.frameCount();
    for (int z = 0; allFrames && z < lightStack.frameCount(); ++z)
        allFrames = sameImage(cal.data.frame(z), processing::subtract(lightStack.frame(z), dp->image()));
    check(allFrames, "calibrated_all.mat: every frame = light(:,:,k) - mean dark");
    const QImage png(out("light.png"));
    check(png.size() == QSize(lp->image().width(), lp->image().height()), "light.png: full resolution");
    check(QImage(out("calibrated.png")) == cp->displayedImage().convertToFormat(QImage(out("calibrated.png")).format()),
          "calibrated.png: same pixels as displayed");

    // ---- new tab + drag and drop
    std::printf("tabs and drag & drop\n");
    SessionTab* second = w.newTab();
    waitUntilIdle(w, 1000);
    check(!second->lightPanel()->hasImage() && w.currentTab() == second, "new tab starts empty");
    check(second->layoutMode() == LayoutMode::Single && sw->mode() == LayoutMode::Single
              && tab->layoutMode() == LayoutMode::Triple,
          "a new tab is in single image mode; the first tab keeps three panels");
    auto* tabs = w.findChild<QTabWidget*>();
    tabs->setCurrentIndex(0);
    QCoreApplication::processEvents();
    check(sw->mode() == LayoutMode::Triple, "the switch follows the tab on show (three panels)");
    tabs->setCurrentIndex(1);
    QCoreApplication::processEvents();
    check(sw->mode() == LayoutMode::Single, "the switch follows the tab on show (single image)");
    {
        const bool accepted = dropFiles(second->lightPanel(), {light});
        waitUntilIdle(w, 120000);
        check(accepted && second->lightPanel()->hasImage(), "dropping a .mat file onto the single panel loads it");
    }
    second->setLayoutMode(LayoutMode::Triple);
    waitUntilIdle(w, 1000);
    {
        const bool accepted = dropFiles(second->lightPanel(), {light});
        waitUntilIdle(w, 120000);
        check(accepted && second->lightPanel()->hasImage() && !second->calibratedPanel()->hasImage(),
              "dropping a .mat file onto Light frames loads it");
        check(!dropFiles(second->lightPanel(), {light, dark}, false), "two .mat files at once are refused");
        check(dropFiles(second->calibratedPanel(), {light}, false), "Calibrated frames accepts one dropped file");
    }
    w.closeTab(1);
    waitUntilIdle(w, 1000);

    if (useFixtures) {
        const auto fixture = [](const char* name) { return QStringLiteral(IAT_TEST_DATA_DIR "/") + QLatin1String(name); };
        const ImageStack lightMat = MatFileReader::read(light).imageStacks().first().data;
        const ImageStack darkMat = MatFileReader::read(dark).imageStacks().first().data;

        // ---- Single frame / Mean
        std::printf("single frame / mean\n");
        check(lp->frameMode() == FrameMode::Single && dp->frameMode() == FrameMode::Mean,
              "defaults: light Single frame, dark Mean");
        check(lp->frameCount() == 4 && dp->frameCount() == 3, "light and dark panels both have a frame selector");
        lp->setCurrentFrame(1);
        dp->setFrameMode(FrameMode::Single);
        dp->setCurrentFrame(2);
        waitUntilIdle(w, 5000);
        check(near(dp->meanStd().mean, 24.25) && sameImage(dp->image(), darkMat.frame(2)),
              "dark Single frame shows dark(:, :, 3)");
        check(sameImage(cp->image(), processing::subtract(lightMat.frame(1), darkMat.frame(2)))
                  && near(cp->meanStd().mean, 1261.75),
              "both single: calibrated = light frame 2 - dark frame 3");
        check(cp->sourceInfo().contains(QLatin1String("dark frame 3 of 3")), "calibrated source line names the dark frame");
        check(near(cp->noise(), 1.118033988750) && near(dp->noise(), 8.164965809277),
              "noise values unchanged (whole stacks)");
        lp->setFrameMode(FrameMode::Mean);
        waitUntilIdle(w, 5000);
        check(near(lp->meanStd().mean, 1286.5) && sameImage(lp->image(), processing::meanOverFrames(lightMat))
                  && lp->sourceInfo().contains(QLatin1String("mean of 4 frames")),
              "light Mean shows mean(light, 3)");
        check(sameImage(cp->image(), processing::subtract(processing::meanOverFrames(lightMat), darkMat.frame(2))),
              "calibrated = mean light - dark frame 3");
        lp->stepFrame(+1);
        check(lp->currentFrame() == 1, "frame buttons do nothing in Mean mode");

        tab->exportTo(lp, ExportKind::FrameMat, out("light_mean.mat"));
        tab->exportTo(dp, ExportKind::FrameMat, out("dark_frame.mat"));
        tab->exportTo(dp, ExportKind::StackMat, out("dark_all.mat"));
        tab->exportTo(cp, ExportKind::FrameMat, out("calibrated_modes.mat"));
        check(waitUntilIdle(w, 60000), "exports finished");
        const MatFileContents lm = MatFileReader::read(out("light_mean.mat"));
        check(lm.ok() && sameImage(variable(lm, "light_mean").data.frame(0), lp->image())
                  && scalar(lm, "light_frame_count") == 4.0,
              "light_mean.mat: mean image + frame count");
        const MatFileContents df = MatFileReader::read(out("dark_frame.mat"));
        check(df.ok() && sameImage(variable(df, "dark_frame").data.frame(0), darkMat.frame(2))
                  && scalar(df, "frame_index") == 3.0 && scalar(df, "dark_frame_count") == 3.0,
              "dark_frame.mat: dark frame 3 + index");
        const MatFileContents da = MatFileReader::read(out("dark_all.mat"));
        check(da.ok() && variable(da, "dark").data.rawData() == darkMat.rawData(), "dark_all.mat: identical to the input");
        const MatFileContents cm = MatFileReader::read(out("calibrated_modes.mat"));
        check(cm.ok() && sameImage(variable(cm, "calibrated_frame").data.frame(0), cp->image())
                  && textLength(cm, "calibration") == qint64(QByteArray("mean(light, 3) - dark(:, :, 3)").size())
                  && scalar(cm, "dark_frame_index") == 3.0 && scalar(cm, "light_frame_count") == 4.0,
              "calibrated .mat: image + formula + frame numbers");
        lp->setFrameMode(FrameMode::Single);
        dp->setFrameMode(FrameMode::Mean);
        waitUntilIdle(w, 5000);
        check(near(cp->meanStd().mean, 1271.75), "back to light frame 2 - mean dark");

        // ---- several image files
        std::printf("image files\n");
        SessionTab* imgTab = w.newTab();
        imgTab->setLayoutMode(LayoutMode::Triple);
        ImagePanel* ilp = imgTab->lightPanel();
        ImagePanel* idp = imgTab->darkPanel();
        ImagePanel* icp = imgTab->calibratedPanel();
        const QStringList pngs{fixture("light_png/light_4.png"), fixture("light_png/light_2.png"),
                               fixture("light_png/light_3.png"), fixture("light_png/light_1.png")};
        const QStringList tiffs{fixture("dark_tiff/dark_10.tif"), fixture("dark_tiff/dark_1.tif"),
                                fixture("dark_tiff/dark_2.tif")};
        check(!dropFiles(ilp, {pngs.first(), light}, false), ".mat and images together are refused");
        check(!dropFiles(icp, {pngs.at(0), pngs.at(1)}, false), "several images onto Calibrated frames are refused");
        const bool okLight = dropFiles(ilp, pngs);
        const bool okDark = dropFiles(idp, tiffs);
        waitUntilIdle(w, 60000);
        check(okLight && ilp->frameCount() == 4 && ilp->sourceInfo().contains(QLatin1String("4 PNG files")),
              "4 PNG files dropped onto Light frames: 4 frames");
        check(okDark && idp->frameCount() == 3 && near(idp->meanStd().mean, 14.25) && near(idp->noise(), 8.164965809277),
              "3 TIFF files dropped onto Dark frames: mean and noise as dark_small.mat");
        check(near(ilp->meanStd().mean, 1285.0) && near(ilp->noise(), 1.118033988750)
                  && near(icp->meanStd().mean, 1270.75) && near(icp->noise(), 1.118033988750),
              "same results as the .mat files (frames in natural order)");
        ilp->setFrameMode(FrameMode::Mean);
        waitUntilIdle(w, 5000);
        check(near(ilp->meanStd().mean, 1286.5) && near(icp->meanStd().mean, 1286.5 - 14.25),
              "Mean of the PNG files");
        w.closeTab(1);  // the image tab
        waitUntilIdle(w, 1000);

        // ---- calibrated image loaded from a file
        std::printf("calibrated image from a file\n");
        auto* indicator = cp->findChild<QToolButton*>(QStringLiteral("autoCalibrationOff"));
        check(indicator && !indicator->isVisible() && tab->isAutoCalibration(), "automatic calibration is on at first");
        const bool okCal = dropFiles(cp, {fixture("light_png/light_3.png")});
        waitUntilIdle(w, 60000);
        check(okCal && !tab->isAutoCalibration() && cp->isAutoCalibrationOff() && indicator->isVisible(),
              "dropping an image onto Calibrated frames turns auto calibration off (red indicator)");
        check(sameImage(cp->image(), lightMat.frame(2)), "the calibrated panel shows the loaded image");
        if (!shot.isEmpty()) {
            QString manualShot = shot;
            manualShot.replace(QLatin1String(".png"), QLatin1String("_manual.png"));
            w.grab().save(manualShot);
        }
        lp->setCurrentFrame(3);
        dp->setFrameMode(FrameMode::Single);
        waitUntilIdle(w, 5000);
        check(sameImage(cp->image(), lightMat.frame(2)), "light and dark changes leave it alone");
        tab->exportTo(cp, ExportKind::FrameMat, out("calibrated_loaded.mat"));
        waitUntilIdle(w, 60000);
        const MatFileContents cl = MatFileReader::read(out("calibrated_loaded.mat"));
        check(cl.ok() && sameImage(variable(cl, "calibrated_frame").data.frame(0), lightMat.frame(2))
                  && textLength(cl, "source_file") > 0 && variable(cl, "light_file").name.isEmpty(),
              "export: the loaded image and its file name");
        indicator->click();
        waitUntilIdle(w, 5000);
        check(tab->isAutoCalibration() && !cp->isAutoCalibrationOff() && !indicator->isVisible()
                  && sameImage(cp->image(), processing::subtract(lightMat.frame(3), darkMat.frame(2)))
                  && near(cp->noise(), 1.118033988750),
              "clicking the red indicator: back to light frame 4 - dark frame 3");

        tab->loadCalibrated(dark);  // a stack: the calibrated panel gets a frame selector
        waitUntilIdle(w, 60000);
        cp->stepFrame(+1);
        waitUntilIdle(w, 5000);
        check(!tab->isAutoCalibration() && cp->frameCount() == 3 && sameImage(cp->image(), darkMat.frame(1))
                  && near(cp->noise(), 8.164965809277),
              "a loaded .mat stack can be stepped through");
        tab->restoreAutoCalibration();
        dp->setFrameMode(FrameMode::Mean);
        lp->setCurrentFrame(0);
        waitUntilIdle(w, 5000);
        check(cp->frameCount() == 0 && near(cp->meanStd().mean, 1270.75), "restored: light frame 1 - mean dark");
    }

    // ---- image processing pipeline of a panel
    std::printf("image processing\n");
    {
        using isp::StageId;
        waitUntilIdle(w, 5000);
        const Image2D raw = lp->image();
        const double rawMean = lp->meanStd().mean, rawLow = lp->displayLow(), rawHigh = lp->displayHigh();
        const Image2D calBefore = cp->image();
        const Image2D darkBefore = dp->image();
        auto* button = lp->findChild<QToolButton*>(QStringLiteral("processButton"));
        check(button && dp->findChild<QToolButton*>(QStringLiteral("processButton"))
                  && cp->findChild<QToolButton*>(QStringLiteral("processButton")) && !lp->isProcessed()
                  && !lp->processingActive() && lp->processingSummary().isEmpty(),
              "every panel has a Processing… button; processing is off");
        if (button)
            button->click();
        QCoreApplication::processEvents();
        PipelineDialog* dlg = lp->pipelineDialog();
        const auto nodes = dlg->findChildren<QWidget*>(QRegularExpression(QStringLiteral("^stageNode_")));
        check(dlg->isVisible() && nodes.size() == int(isp::stages().size()) && nodes.size() == 10
                  && !dlg->findChild<QWidget*>(QStringLiteral("stageNode_white_balance"))
                  && dlg->windowTitle().contains(lp->title()),
              "the button opens the pipeline: one node per stage (no white balance)");
        check(sameImage(lp->image(), raw) && !w.isBusy(), "opening it changes nothing: every stage is bypassed");

        // The switch on a node turns its stage on.
        QWidget* toneNode = dlg->findChild<QWidget*>(QStringLiteral("stageNode_tone_map"));
        if (toneNode) {
            const QPointF pos(toneNode->width() - 9 - 18, 8 + 12);  // on the switch
            QMouseEvent press(QEvent::MouseButtonPress, pos, toneNode->mapToGlobal(pos), Qt::LeftButton, Qt::LeftButton,
                              Qt::NoModifier);
            QCoreApplication::sendEvent(toneNode, &press);
        }
        check(dlg->pipeline().isEnabled(StageId::ToneMap) && dlg->selectedStage() == StageId::ToneMap
                  && lp->pipeline() == dlg->pipeline() && w.isBusy(),
              "clicking a node's switch turns the stage on; it runs in the background");
        waitUntilIdle(w, 60000);
        check(lp->isProcessed() && lp->processingActive() && sameImage(lp->rawImage(), raw)
                  && !sameImage(lp->image(), raw),
              "the panel shows the processed image and keeps the original");
        check(lp->stats().min >= 0.0 && lp->stats().max <= 255.0 && lp->displayLow() == 0.0
                  && lp->displayHigh() == 255.0,
              "tone-mapped: display values, shown from 0 to 255");
        check(lp->processingSummary() == QLatin1String("Tone mapping: Percentile stretch"),
              "summary: Tone mapping: Percentile stretch");
        check(sameImage(cp->image(), calBefore) && sameImage(dp->image(), darkBefore) && !dp->isProcessed()
                  && !cp->isProcessed(),
              "the other panels (and the calibration) still use unprocessed images");

        // Choosing an algorithm and moving a parameter slider.
        auto* algCombo = dlg->findChild<QComboBox*>(QStringLiteral("stageAlgorithm"));
        const int clahe = algCombo ? algCombo->findText(QStringLiteral("CLAHE")) : -1;
        if (clahe >= 0) {
            algCombo->setCurrentIndex(clahe);
            emit algCombo->activated(clahe);
        }
        waitUntilIdle(w, 60000);
        check(clahe >= 0 && lp->processingSummary() == QLatin1String("Tone mapping: CLAHE")
                  && int(algCombo->count()) == int(isp::stage(StageId::ToneMap).algorithms.size()),
              "the algorithm list selects CLAHE");
        const Image2D claheImage = lp->image();
        auto* tiles = dlg->findChild<QSlider*>(QStringLiteral("param_tiles"));
        if (tiles)
            tiles->setValue(2);
        waitUntilIdle(w, 60000);
        check(tiles && lp->pipeline().value(StageId::ToneMap, "tiles") == 2.0 && !sameImage(lp->image(), claheImage),
              "a parameter slider changes the result");

        // A second stage; the display range is kept, since the scale is the same.
        lp->setDisplayRange(10.0, 200.0);
        dlg->setStageEnabled(StageId::Sharpen, true);
        waitUntilIdle(w, 60000);
        check(lp->processingSummary() == QLatin1String("Tone mapping: CLAHE; Sharpening: Unsharp mask")
                  && lp->displayLow() == 10.0 && lp->displayHigh() == 200.0,
              "sharpening added; the display range is kept");

        // Another frame is processed with the same settings.
        const Image2D processedFrame = lp->image();
        lp->stepFrame(+1);
        waitUntilIdle(w, 60000);
        check(lp->isProcessed() && !sameImage(lp->rawImage(), raw) && lp->image().sameSize(raw)
                  && lp->stats().max <= 255.0,
              "the next frame is processed too");
        lp->stepFrame(-1);
        waitUntilIdle(w, 60000);
        check(sameImage(lp->rawImage(), raw) && sameImage(lp->image(), processedFrame), "and back: the same result");

        // Exports: the processed image, and which stages were applied.
        tab->exportTo(lp, ExportKind::FrameMat, out("light_processed.mat"));
        tab->exportTo(lp, ExportKind::StackMat, out("light_all_unprocessed.mat"));
        waitUntilIdle(w, 60000);
        const MatFileContents pf = MatFileReader::read(out("light_processed.mat"));
        const MatVariable processedVar = variable(pf, lp->frameMode() == FrameMode::Mean ? "light_mean" : "light_frame");
        check(pf.ok() && sameImage(processedVar.data.frame(0), lp->image())
                  && textLength(pf, "processing") == lp->processingSummary().size(),
              ".mat of the image on display: processed values + the stages");
        const MatFileContents pa = MatFileReader::read(out("light_all_unprocessed.mat"));
        check(pa.ok() && textLength(pa, "processing") < 0 && sameImage(variable(pa, "light").data.frame(lp->currentFrame()), raw),
              ".mat of all frames: the unprocessed stack");

        // Show original: a quick comparison that keeps the settings.
        dlg->setShowOriginal(true);
        waitUntilIdle(w, 60000);
        check(lp->showOriginal() && !lp->isProcessed() && sameImage(lp->image(), raw) && near(lp->meanStd().mean, rawMean)
                  && lp->pipeline().isActive() && lp->displayHigh() != 200.0,
              "Show original: the unprocessed image, contrast set for its values");
        dlg->setShowOriginal(false);
        waitUntilIdle(w, 60000);
        check(lp->isProcessed() && sameImage(lp->image(), processedFrame) && lp->displayLow() == 0.0
                  && lp->displayHigh() == 255.0,
              "and off again: the processed image, shown 0 to 255");

        // A stage that keeps the scale (denoise) on its own keeps DN values.
        dlg->resetAll();
        waitUntilIdle(w, 60000);
        check(!lp->pipeline().isActive() && !lp->isProcessed() && sameImage(lp->image(), raw),
              "Reset all: back to the unprocessed image");
        lp->setDisplayRange(rawLow, rawHigh);
        dlg->applyPreset("cleanup");
        waitUntilIdle(w, 60000);
        check(lp->isProcessed() && lp->displayLow() == rawLow && lp->displayHigh() == rawHigh
                  && lp->processingSummary().startsWith(QLatin1String("Defect pixels")),
              "preset 'Sensor clean-up only': still DN, display range kept");

        // A/B: a divider across the image, unprocessed left, processed right.
        {
            auto* ab = lp->findChild<QToolButton*>(QStringLiteral("abButton"));
            auto* abInDialog = dlg->findChild<QCheckBox*>(QStringLiteral("abCompare"));
            ImageView* view = lp->view();
            auto* dpAb = dp->findChild<QToolButton*>(QStringLiteral("abButton"));
            check(ab && abInDialog && ab->isEnabled() && !ab->isChecked() && !view->hasCompareImage()
                      && dpAb && !dpAb->isEnabled(),
                  "A/B button: enabled where processing is on, off at first");
            dlg->applyPreset("general");  // tone mapped: the two sides differ clearly
            waitUntilIdle(w, 60000);
            const QImage plain = view->viewport()->grab().toImage();
            if (ab)
                ab->click();
            QCoreApplication::processEvents();
            const int x0 = view->compareDividerX();
            check(lp->abCompare() && view->hasCompareImage() && abInDialog->isChecked()
                      && view->comparePosition() == 0.5 && x0 > 0,
                  "A/B on: the divider is in the middle of the image");
            // Left of the divider the view shows the unprocessed image, right
            // of it the picture is unchanged.
            const QImage split = view->viewport()->grab().toImage();
            const int yMid = split.height() / 2;
            auto rowDiffers = [&](const QImage& a, const QImage& b, int from, int to) {
                for (int x = from; x < to; ++x)
                    if (a.pixel(x, yMid - 20) != b.pixel(x, yMid - 20))
                        return true;
                return false;
            };
            const QRect vis = view->mapFromScene(view->sceneRect()).boundingRect().intersected(view->viewport()->rect());
            check(rowDiffers(split, plain, vis.left() + 2, x0 - 14) && !rowDiffers(split, plain, x0 + 14, vis.right() - 2),
                  "left of it the unprocessed image, right of it the processed one");

            // Dragging the divider moves it and does not pan the image.
            const ViewState before = view->viewState();
            auto mouse = [&](QEvent::Type type, const QPoint& pos, Qt::MouseButton button, Qt::MouseButtons buttons) {
                QMouseEvent e(type, QPointF(pos), QPointF(view->viewport()->mapToGlobal(pos)), button, buttons,
                              Qt::NoModifier);
                QCoreApplication::sendEvent(view->viewport(), &e);
            };
            const int x1 = vis.left() + vis.width() / 4;
            mouse(QEvent::MouseButtonPress, QPoint(x0, yMid), Qt::LeftButton, Qt::LeftButton);
            mouse(QEvent::MouseMove, QPoint(x1, yMid), Qt::NoButton, Qt::LeftButton);
            mouse(QEvent::MouseButtonRelease, QPoint(x1, yMid), Qt::LeftButton, Qt::NoButton);
            QCoreApplication::processEvents();
            const ViewState after = view->viewState();
            check(std::abs(view->comparePosition() - 0.25) < 0.02 && std::abs(view->compareDividerX() - x1) <= 1
                      && after.fit == before.fit && after.center == before.center,
                  "dragging the divider moves it to a quarter of the image; the image stays put");
            const QImage moved = view->viewport()->grab().toImage();
            check(!rowDiffers(moved, plain, x1 + 14, vis.right() - 2) && rowDiffers(moved, plain, vis.left() + 2, x1 - 14),
                  "and the two sides follow it");

            // The comparison follows the processing state.
            dlg->setShowOriginal(true);
            waitUntilIdle(w, 60000);
            check(!view->hasCompareImage() && !ab->isEnabled() && lp->abCompare() && !abInDialog->isEnabled(),
                  "Show original: nothing to compare, the divider goes (the choice is kept)");
            dlg->setShowOriginal(false);
            waitUntilIdle(w, 60000);
            check(view->hasCompareImage() && ab->isEnabled() && std::abs(view->comparePosition() - 0.25) < 0.02,
                  "back: the divider returns where it was");
            lp->stepFrame(+1);
            waitUntilIdle(w, 60000);
            check(view->hasCompareImage(), "it stays for the next frame");
            lp->stepFrame(-1);
            waitUntilIdle(w, 60000);

            QString abShot = shot;
            if (!abShot.isEmpty()) {
                dlg->applyPreset("hdr");
                waitUntilIdle(w, 60000);
                view->setComparePosition(0.5);
                QCoreApplication::processEvents();
                abShot.replace(QLatin1String(".png"), QLatin1String("_ab.png"));
                if (lp->grab().save(abShot))
                    std::printf("A/B comparison: %s\n", qPrintable(abShot));
            }
            if (abInDialog)
                abInDialog->setChecked(false);
            QCoreApplication::processEvents();
            check(!lp->abCompare() && !view->hasCompareImage() && !ab->isChecked(),
                  "A/B off (from the dialog): the divider is gone");
            view->setComparePosition(0.5);
        }

        QString shotPath = shot;
        if (!shotPath.isEmpty()) {
            dlg->applyPreset("hdr");
            dlg->selectStage(StageId::ToneMap);
            waitUntilIdle(w, 60000);
            dlg->resize(1380, 500);  // the whole chain, whatever the size of the (off-screen) screen
            QCoreApplication::processEvents();
            shotPath.replace(QLatin1String(".png"), QLatin1String("_processing.png"));
            if (dlg->grab().save(shotPath))
                std::printf("processing dialog: %s\n", qPrintable(shotPath));
        }
        dlg->resetAll();
        waitUntilIdle(w, 60000);
        dlg->close();
        check(!dlg->isVisible() && sameImage(lp->image(), raw) && sameImage(cp->image(), calBefore),
              "closed; everything as before");
    }

    // ---- header buttons: icon and text where there is room, icon only when narrow
    std::printf("header buttons\n");
    {
        auto* process = lp->findChild<QToolButton*>(QStringLiteral("processButton"));
        const bool tripleCompact = lp->width() < 640 && process->toolButtonStyle() == Qt::ToolButtonIconOnly;
        check(process && !process->icon().isNull() && !process->toolTip().isEmpty()
                  && process->text() == QStringLiteral("Processing…") && tripleCompact,
              "three panels side by side: Processing… shows its icon only (text and tooltip kept)");
        const LayoutMode modeBefore = tab->layoutMode();
        tab->setLayoutMode(LayoutMode::Single);
        QCoreApplication::processEvents();
        check(lp->width() >= 640 && process->toolButtonStyle() == Qt::ToolButtonTextBesideIcon,
              "single image (wide panel): icon and text");
        // The icons are drawn in the colour of the button text.
        const QImage glyph = process->icon().pixmap(32, 32).toImage();
        int inked = 0;
        for (int y = 0; y < glyph.height(); ++y)
            for (int x = 0; x < glyph.width(); ++x)
                inked += qAlpha(glyph.pixel(x, y)) > 128;
        check(inked > 40 && inked < 600, "the icon is a line drawing on a transparent background");
        tab->setLayoutMode(modeBefore);
        waitUntilIdle(w, 60000);
    }

    // ---- theme: light / dark
    std::printf("theme\n");
    {
        using theme::Mode;
        const QColor lightWindow = QApplication::palette().color(QPalette::Window);
        check(theme::activeMode() == Mode::System && !theme::isDark() && !theme::styleName().isEmpty(),
              "starts with the system's colours (light here)");
        const QImage figureLight = lp->renderReport();
        const QImage windowLight = w.grab().toImage();

        theme::apply(Mode::Dark);
        QCoreApplication::processEvents();
        const QPalette dark = QApplication::palette();
        check(theme::isDark() && theme::activeMode() == Mode::Dark && dark.color(QPalette::Window).lightness() < 90
                  && dark.color(QPalette::WindowText).lightness() > 180,
              "Dark: dark window colour, light text");
        // Widgets take their colours from the palette, so open windows change at once.
        const QImage windowDark = w.grab().toImage();
        const QPoint corner(windowDark.width() - 12, windowDark.height() - 30);  // empty window background
        auto* source = lp->findChild<QLabel*>();
        bool mutedFollows = false;
        for (QLabel* label : lp->findChildren<QLabel*>())
            mutedFollows = mutedFollows || (label->foregroundRole() == QPalette::PlaceholderText
                                            && label->palette().color(label->foregroundRole()).lightness() > 120);
        check(source && QColor(windowDark.pixel(corner)).lightness() < 90
                  && QColor(windowLight.pixel(corner)).lightness() > 200 && mutedFollows
                  && lp->palette().color(QPalette::Window) == dark.color(QPalette::Window),
              "the open window is redrawn dark, secondary text included");
        check(lp->renderReport() == figureLight, "the copied figure stays on white, histogram included");
        if (!shot.isEmpty()) {
            QString darkShot = shot;
            darkShot.replace(QLatin1String(".png"), QLatin1String("_dark.png"));
            lp->pipelineDialog()->applyPreset("hdr");
            lp->setAbCompare(true);
            waitUntilIdle(w, 60000);
            if (w.grab().save(darkShot))
                std::printf("dark theme: %s\n", qPrintable(darkShot));
            lp->pipelineDialog()->resize(1380, 500);
            darkShot.replace(QLatin1String("_dark.png"), QLatin1String("_dark_processing.png"));
            lp->pipelineDialog()->grab().save(darkShot);
            lp->setAbCompare(false);
            lp->pipelineDialog()->resetAll();
            waitUntilIdle(w, 60000);
        }

        // File > Settings offers the three choices (the language list stays first).
        SettingsDialog settings(&w);
        auto* themeCombo = settings.findChild<QComboBox*>(QStringLiteral("theme"));
        check(themeCombo && themeCombo->count() == 3 && settings.themeMode() == theme::savedMode()
                  && settings.findChild<QComboBox*>() == settings.findChild<QComboBox*>(QStringLiteral("language")),
              "Settings: theme list with Follow the system, Light, Dark");
        settings.setThemeMode(Mode::Dark);
        check(settings.themeMode() == Mode::Dark && themeCombo->currentText() == QLatin1String("Dark"),
              "the dialog reports the chosen theme");

        theme::apply(Mode::Light);
        QCoreApplication::processEvents();
        check(!theme::isDark() && QApplication::palette().color(QPalette::Window).lightness() > 200, "Light: light colours");
        theme::apply(Mode::System);
        QCoreApplication::processEvents();
        check(!theme::isDark() && theme::activeMode() == Mode::System
                  && QApplication::palette().color(QPalette::Window) == lightWindow,
              "Follow the system: back to the colours it started with");
    }

    // ---- icon and author / GitHub
    std::printf("icon and GitHub\n");
    const QString url = QStringLiteral("https://github.com/AuroralDiffraction");
    check(!QApplication::windowIcon().isNull() && QApplication::windowIcon().availableSizes().size() == 7,
          "app icon loaded from resources (7 sizes)");
    check(AboutDialog::homepage() == url && AboutDialog::author() == QLatin1String("AuroralDiffraction (Tanatonium)"),
          "author and homepage from CMakeLists");
    bool statusLink = false;
    for (const QLabel* l : w.statusBar()->findChildren<QLabel*>())
        statusLink = statusLink || l->text().contains(QLatin1String("github"), Qt::CaseInsensitive);
    check(!statusLink, "no GitHub link in the status bar");
    QImage aboutShot;
    {
        AboutDialog about(&w);
        about.show();
        QCoreApplication::processEvents();
        bool link = false;
        for (const QLabel* l : about.findChildren<QLabel*>())
            link = link || (l->text().contains(url) && l->openExternalLinks());
        check(link, "About dialog has a clickable GitHub link");
        aboutShot = about.grab().toImage();
    }

    // ---- interface language
    std::printf("language\n");
    check(language::defaultCode() == QLatin1String("zh_CN") && language::languages().size() == 2
              && language::activeCode() == QLatin1String("en"),
          "two languages; Chinese is the default");
    check(language::install(QStringLiteral("zh_CN")) && language::activeCode() == QLatin1String("zh_CN"),
          "the Chinese translation is built into the program");
    {
        MainWindow zh;
        zh.resize(1600, 1000);
        zh.show();
        QCoreApplication::processEvents();
        SessionTab* zt = zh.currentTab();
        const QList<QAction*> menus = zh.menuBar()->actions();
        check(zt->lightPanel()->title() == QStringLiteral("图像") && !menus.isEmpty()
                  && menus.first()->text() == QStringLiteral("文件(&F)"),
              "a window built now has Chinese texts");
        zt->loadImage(light);
        waitUntilIdle(zh, 120000);
        check(zt->lightPanel()->hasImage() && zt->lightPanel()->sourceInfo().contains(QStringLiteral("light"))
                  && near(zt->lightPanel()->meanStd().mean, lp->frameMode() == FrameMode::Single
                                                                 ? processing::calculateImageStats(
                                                                       MatFileReader::read(light).imageStacks().first().data.frame(0)).mean
                                                                 : zt->lightPanel()->meanStd().mean),
              "loading and the calculations work the same");
        if (useFixtures) {
            const QString paletteTiff = QStringLiteral(IAT_TEST_DATA_DIR "/formats/palette.tif");
            const QString v73 = QStringLiteral(IAT_TEST_DATA_DIR "/v73_header.mat");
            check(ImageFileReader::read(paletteTiff).error.contains(QStringLiteral("调色板"))
                      && MatFileReader::read(v73).error.contains(QStringLiteral("暂不支持")),
                  "error messages of the file readers are translated");
        }
        {
            // The image processing pipeline: its own texts, and the stage and
            // algorithm names from the tables in core/Isp.h.
            ImagePanel* zp = zt->lightPanel();
            PipelineDialog* zd = zp->pipelineDialog();
            auto* zb = zp->findChild<QToolButton*>(QStringLiteral("processButton"));
            zd->applyPreset("inspection");
            waitUntilIdle(zh, 60000);
            auto* zc = zd->findChild<QComboBox*>(QStringLiteral("stageAlgorithm"));
            const QStringList notes = isp::run(zp->rawImage(), zp->pipeline()).notes;
            check(zb && zb->text() == QStringLiteral("图像处理…") && zd->windowTitle() == QStringLiteral("图像处理：图像")
                      && isp::text(isp::stage(isp::StageId::ToneMap).name) == QStringLiteral("色调映射")
                      && zc && zc->findText(QStringLiteral("曝光融合")) >= 0
                      && zp->pipeline().summary(true) == QStringLiteral("坏点校正: 邻域最小 / 最大值; 降噪: 双边滤波; 色调映射: CLAHE")
                      && zp->processingSummary() == QLatin1String("Defect pixels: Neighbour min / max; Denoise: Bilateral filter; Tone mapping: CLAHE")
                      && notes.value(isp::stageIndex(isp::StageId::DefectPixel)).startsWith(QStringLiteral("已校正")),
                  "the image processing pipeline is translated (files keep English)");
            if (!shot.isEmpty()) {
                zd->show();
                QCoreApplication::processEvents();
                QString zhShot = shot;
                zhShot.replace(QLatin1String(".png"), QLatin1String("_zh_processing.png"));
                zd->grab().save(zhShot);
                zd->close();
            }
            zd->resetAll();
            waitUntilIdle(zh, 60000);
        }
        SettingsDialog settings(&zh);
        auto* combo = settings.findChild<QComboBox*>();
        check(combo && combo->count() == 2 && combo->itemText(0) == QStringLiteral("简体中文")
                  && combo->itemText(1) == QLatin1String("English") && settings.windowTitle() == QStringLiteral("设置"),
              "Settings lists the languages under their own names");
        if (!shot.isEmpty()) {
            QString zhShot = shot;
            zhShot.replace(QLatin1String(".png"), QLatin1String("_zh_single.png"));
            zh.statusBar()->clearMessage();
            zh.grab().save(zhShot);
            zt->setLayoutMode(LayoutMode::Triple);
            zt->loadDark(dark);
            waitUntilIdle(zh, 120000);
            zhShot.replace(QLatin1String("_zh_single.png"), QLatin1String("_zh.png"));
            zh.grab().save(zhShot);
            settings.show();
            QCoreApplication::processEvents();
            zhShot.replace(QLatin1String("_zh.png"), QLatin1String("_zh_settings.png"));
            settings.grab().save(zhShot);
        }
    }
    language::install(QStringLiteral("en"));
    {
        MainWindow en;
        check(en.currentTab()->lightPanel()->title() == QLatin1String("Image"), "English again after switching back");
    }

    if (!shot.isEmpty()) {
        QString aboutPath = shot;
        aboutPath.replace(QLatin1String(".png"), QLatin1String("_about.png"));
        if (aboutShot.save(aboutPath))
            std::printf("about dialog: %s\n", qPrintable(aboutPath));
        w.statusBar()->clearMessage();  // messages left by the checks above
        if (w.grab().save(shot))
            std::printf("screenshot: %s\n", qPrintable(shot));
        QString reportPath = shot;
        reportPath.replace(QLatin1String(".png"), QLatin1String("_report.png"));
        if (lp->renderReport().save(reportPath))
            std::printf("report image: %s\n", qPrintable(reportPath));
    }

    std::printf("%s\n", g_ok ? "PASS" : "FAIL");
    return g_ok ? 0 : 1;
}
