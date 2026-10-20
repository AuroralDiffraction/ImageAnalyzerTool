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
    const QCommandLineOption lightOption({QStringLiteral("l"), QStringLiteral("light")},
                                         QStringLiteral("Light frames .mat file to open in the first tab."),
                                         QStringLiteral("file"));
    const QCommandLineOption darkOption({QStringLiteral("d"), QStringLiteral("dark")},
                                        QStringLiteral("Dark frames .mat file to open in the first tab."),
                                        QStringLiteral("file"));
    parser.addOption(lightOption);
    parser.addOption(darkOption);
    parser.process(app);

    iat::MainWindow window;
    window.show();

    if (iat::SessionTab* tab = window.currentTab()) {
        if (parser.isSet(lightOption))
            tab->loadLight(parser.value(lightOption));
        if (parser.isSet(darkOption))
            tab->loadDark(parser.value(darkOption));
    }

    return app.exec();
}
