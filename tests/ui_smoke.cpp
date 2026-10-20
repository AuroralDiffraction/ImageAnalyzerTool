// UI smoke test: opens the main window, loads light and dark frames into the
// first tab, waits for loading to finish and checks the results, the frame
// buttons, linked zoom/pan, the clipboard copies and the exports.
//
//   ui_smoke                                  uses the small test files
//   ui_smoke light.mat dark.mat [shot.png] [frame]
//
// Exported files go to a temporary folder, or to IAT_SMOKE_EXPORT_DIR if set.
// Run headless with QT_QPA_PLATFORM=offscreen.

#include "core/Processing.h"
#include "io/MatFileReader.h"
#include "ui/AboutDialog.h"
#include "ui/ImagePanel.h"
#include "ui/ImageView.h"
#include "ui/MainWindow.h"
#include "ui/SessionTab.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QIcon>
#include <QLabel>
#include <QMimeData>
#include <QPixmap>
#include <QScrollBar>
#include <QTemporaryDir>
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
    const bool useFixtures = args.size() < 3;
    const QString light = useFixtures ? QStringLiteral(IAT_TEST_DATA_DIR "/light_small.mat") : args.at(1);
    const QString dark = useFixtures ? QStringLiteral(IAT_TEST_DATA_DIR "/dark_small.mat") : args.at(2);
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
    tab->loadLight(light);
    tab->loadDark(dark);
    if (!waitUntilIdle(w, 120000)) {
        std::printf("FAIL: loading did not finish\n");
        return 1;
    }
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
    auto out = [&](const char* name) { return QDir(exportDir).filePath(QLatin1String(name)); };
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
    {
        QMimeData dropMime;
        dropMime.setUrls({QUrl::fromLocalFile(light)});
        ImagePanel* target = second->lightPanel();
        const QPoint c = target->rect().center();
        QDragEnterEvent enter(c, Qt::CopyAction, &dropMime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(target, &enter);
        const bool accepted = enter.isAccepted();
        QDropEvent drop(QPointF(c), Qt::CopyAction, &dropMime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(target, &drop);
        waitUntilIdle(w, 120000);
        check(accepted && second->lightPanel()->hasImage() && !second->calibratedPanel()->hasImage(),
              "dropping a .mat file onto Light frames loads it");
        QDragEnterEvent enterCal(c, Qt::CopyAction, &dropMime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(second->calibratedPanel(), &enterCal);
        check(!enterCal.isAccepted(), "Calibrated frames does not accept drops");
    }
    w.closeTab(1);
    waitUntilIdle(w, 1000);

    // ---- icon and author / GitHub
    std::printf("icon and GitHub\n");
    const QString url = QStringLiteral("https://github.com/AuroralDiffraction");
    check(!QApplication::windowIcon().isNull() && QApplication::windowIcon().availableSizes().size() == 7,
          "app icon loaded from resources (7 sizes)");
    check(AboutDialog::homepage() == url && AboutDialog::author() == QLatin1String("AuroralDiffraction (Tanatonium)"),
          "author and homepage from CMakeLists");
    const QLabel* credit = w.findChild<QLabel*>(QStringLiteral("githubLink"));
    check(credit && credit->text().contains(url) && credit->openExternalLinks(), "status bar shows a GitHub link");
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

    if (!shot.isEmpty()) {
        QString aboutPath = shot;
        aboutPath.replace(QLatin1String(".png"), QLatin1String("_about.png"));
        if (aboutShot.save(aboutPath))
            std::printf("about dialog: %s\n", qPrintable(aboutPath));
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
