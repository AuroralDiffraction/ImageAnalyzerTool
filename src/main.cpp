#include "AppInfo.h"
#include "ui/Language.h"
#include "ui/MainWindow.h"
#include "ui/SessionTab.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ImageAnalyzerTool"));
    QApplication::setOrganizationName(QStringLiteral("ImageAnalyzerTool"));
    QApplication::setApplicationDisplayName(QStringLiteral(IAT_APP_NAME));
    QApplication::setApplicationVersion(QStringLiteral(IAT_VERSION));

    // Interface language: Chinese unless another one was chosen in File > Settings.
    // Installed before anything else, since texts are translated where they are created.
    iat::language::install(iat::language::savedCode());

    // Window and taskbar icon (the .exe's own icon comes from the Windows resource file).
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addFile(QStringLiteral(":/icons/app_%1.png").arg(size), QSize(size, size));
    QApplication::setWindowIcon(icon);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate(
        "main", "Dark-frame calibration and noise analysis for image stacks (MATLAB .mat, TIFF, PNG, JPEG)"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption lightOption(
        {QStringLiteral("l"), QStringLiteral("light")},
        QCoreApplication::translate("main", "Image to open in the first tab (the light frames, when dark frames are "
                                            "given too): a .mat file, or an image file (repeat the option for "
                                            "several images)."),
        QCoreApplication::translate("main", "file"));
    const QCommandLineOption darkOption(
        {QStringLiteral("d"), QStringLiteral("dark")},
        QCoreApplication::translate("main", "Dark frames to open in the first tab (switches it to three panels): "
                                            "a .mat file, or an image file (repeat the option for several images)."),
        QCoreApplication::translate("main", "file"));
    const QCommandLineOption calibratedOption(
        {QStringLiteral("c"), QStringLiteral("calibrated")},
        QCoreApplication::translate("main", "Calibrated image to show in the first tab (switches it to three "
                                            "panels and turns automatic calibration off)."),
        QCoreApplication::translate("main", "file"));
    parser.addOption(lightOption);
    parser.addOption(darkOption);
    parser.addOption(calibratedOption);
    parser.process(app);

    iat::MainWindow window;
    window.show();

    if (iat::SessionTab* tab = window.currentTab()) {
        // A new tab shows a single image; dark frames or a calibrated image need the three panels.
        if (parser.isSet(darkOption) || parser.isSet(calibratedOption))
            tab->setLayoutMode(iat::LayoutMode::Triple);
        if (parser.isSet(lightOption))
            tab->loadLight(parser.values(lightOption));
        if (parser.isSet(darkOption))
            tab->loadDark(parser.values(darkOption));
        if (parser.isSet(calibratedOption))
            tab->loadCalibrated(parser.value(calibratedOption));
    }

    return app.exec();
}
