#pragma once

#include <QMainWindow>

class QAction;
class QTabWidget;

namespace iat {

class ModeSwitch;
class SessionTab;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    SessionTab* currentTab() const;
    bool isBusy() const;
    // The single image / three panels switch next to the tabs. It shows and
    // changes the mode of the current tab; every tab has its own mode.
    ModeSwitch* modeSwitch() const { return m_modeSwitch; }

public slots:
    SessionTab* newTab();
    void closeTab(int index);

private:
    void renameTab(int index);
    void createMenus();
    void openSettings();
    // Brings the switch and the File menu in line with the current tab's mode.
    void syncModeUi(bool animate);

    QTabWidget* m_tabs = nullptr;
    ModeSwitch* m_modeSwitch = nullptr;
    QAction* m_openLightAction = nullptr;
    QAction* m_openDarkAction = nullptr;
    QAction* m_openCalibratedAction = nullptr;
    int m_tabCounter = 0;
};

} // namespace iat
