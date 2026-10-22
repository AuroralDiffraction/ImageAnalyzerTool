#include "ui/MainWindow.h"

#include "ui/AboutDialog.h"
#include "ui/SessionTab.h"

#include <QAction>
#include <QApplication>
#include <QInputDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>

namespace iat {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Image Analyzer Tool"));
    resize(1600, 950);

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    setCentralWidget(m_tabs);

    auto* addButton = new QToolButton(m_tabs);
    addButton->setText(QStringLiteral("+"));
    addButton->setAutoRaise(true);
    addButton->setToolTip(tr("New tab (Ctrl+T)"));
    QFont f = addButton->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() + 3);
    addButton->setFont(f);
    m_tabs->setCornerWidget(addButton, Qt::TopRightCorner);
    connect(addButton, &QToolButton::clicked, this, &MainWindow::newTab);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabs->tabBar(), &QTabBar::tabBarDoubleClicked, this, [this](int index) {
        if (index < 0)
            newTab();          // double-click on the empty part of the tab bar
        else
            renameTab(index);
    });

    createMenus();

    statusBar()->showMessage(
        tr("Drop a .mat file, or one or more TIFF, PNG or JPG images, onto the Light frames and Dark frames panels."));
    newTab();
}

void MainWindow::createMenus()
{
    QMenu* file = menuBar()->addMenu(tr("&File"));

    QAction* newTabAction = file->addAction(tr("&New tab"), this, &MainWindow::newTab);
    newTabAction->setShortcut(QKeySequence::AddTab);

    file->addSeparator();
    QAction* openLight = file->addAction(tr("Open &light frames…"), this, [this] {
        if (SessionTab* t = currentTab()) t->openLightDialog();
    });
    openLight->setShortcut(QKeySequence(tr("Ctrl+L")));
    QAction* openDark = file->addAction(tr("Open &dark frames…"), this, [this] {
        if (SessionTab* t = currentTab()) t->openDarkDialog();
    });
    openDark->setShortcut(QKeySequence(tr("Ctrl+D")));
    file->addAction(tr("Open &calibrated image…"), this, [this] {
        if (SessionTab* t = currentTab()) t->openCalibratedDialog();
    });

    file->addSeparator();
    QAction* closeTabAction = file->addAction(tr("&Close tab"), this, [this] { closeTab(m_tabs->currentIndex()); });
    closeTabAction->setShortcut(QKeySequence::Close);
    QAction* renameAction = file->addAction(tr("&Rename tab…"), this, [this] { renameTab(m_tabs->currentIndex()); });
    renameAction->setShortcut(QKeySequence(Qt::Key_F2));

    file->addSeparator();
    QAction* quit = file->addAction(tr("E&xit"), qApp, &QApplication::closeAllWindows);
    quit->setShortcut(QKeySequence::Quit);

    QMenu* help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&GitHub: %1").arg(AboutDialog::author()), this, [] { AboutDialog::openHomepage(); });
    help->addSeparator();
    help->addAction(tr("&About Image Analyzer Tool…"), this, [this] {
        AboutDialog dialog(this);
        dialog.exec();
    });
    help->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}

SessionTab* MainWindow::newTab()
{
    auto* tab = new SessionTab(m_tabs);
    const int index = m_tabs->addTab(tab, tr("Session %1").arg(++m_tabCounter));
    m_tabs->setCurrentIndex(index);
    connect(tab, &SessionTab::statusMessage, this, [this](const QString& msg) {
        statusBar()->showMessage(msg, 8000);
    });
    return tab;
}

void MainWindow::closeTab(int index)
{
    QWidget* w = m_tabs->widget(index);
    if (!w)
        return;
    m_tabs->removeTab(index);
    w->deleteLater();
    if (m_tabs->count() == 0)
        newTab();
}

void MainWindow::renameTab(int index)
{
    if (index < 0 || index >= m_tabs->count())
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Rename tab"), tr("Tab name:"), QLineEdit::Normal,
                                               m_tabs->tabText(index), &ok);
    if (ok && !name.trimmed().isEmpty())
        m_tabs->setTabText(index, name.trimmed());
}

SessionTab* MainWindow::currentTab() const
{
    return qobject_cast<SessionTab*>(m_tabs->currentWidget());
}

bool MainWindow::isBusy() const
{
    for (int i = 0; i < m_tabs->count(); ++i)
        if (auto* t = qobject_cast<SessionTab*>(m_tabs->widget(i)); t && t->isBusy())
            return true;
    return false;
}

} // namespace iat
