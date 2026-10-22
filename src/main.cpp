#include "AppInfo.h"
#include "ui/MainWindow.h"
#include "ui/SessionTab.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ImageAnalyzerTool"));
    QApplication::setOrganizationName(QStringLiteral("ImageAnalyzerTool"));
    QApplication::setApplicationDisplayName(QStringLiteral(IAT_APP_NAME));
    QApplication::setApplicationVersion(QStringLiteral(IAT_VERSION));

    // Window and taskbar icon (the .exe's own icon comes from the Windows resource file).
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addFile(QStringLiteral(":/icons/app_%1.png").arg(size), QSize(size, size));
    QApplication::setWindowIcon(icon);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(IAT_DESCRIPTION));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption lightOption(
        {QStringLiteral("l"), QStringLiteral("light")},
        QStringLiteral("Light frames to open in the first tab: a .mat file, or an image file "
                       "(repeat the option for several images)."),
        QStringLiteral("file"));
    const QCommandLineOption darkOption(
        {QStringLiteral("d"), QStringLiteral("dark")},
        QStringLiteral("Dark frames to open in the first tab: a .mat file, or an image file "
                       "(repeat the option for several images)."),
        QStringLiteral("file"));
    const QCommandLineOption calibratedOption(
        {QStringLiteral("c"), QStringLiteral("calibrated")},
        QStringLiteral("Calibrated image to show in the first tab (turns automatic calibration off)."),
        QStringLiteral("file"));
    parser.addOption(lightOption);
    parser.addOption(darkOption);
    parser.addOption(calibratedOption);
    parser.process(app);

    iat::MainWindow window;
    window.show();

    if (iat::SessionTab* tab = window.currentTab()) {
        if (parser.isSet(lightOption))
            tab->loadLight(parser.values(lightOption));
        if (parser.isSet(darkOption))
            tab->loadDark(parser.values(darkOption));
        if (parser.isSet(calibratedOption))
            tab->loadCalibrated(parser.value(calibratedOption));
    }

    return app.exec();
}
