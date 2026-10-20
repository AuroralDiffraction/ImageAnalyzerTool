#pragma once

#include <QMainWindow>

class QTabWidget;

namespace iat {

class SessionTab;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    SessionTab* currentTab() const;
    bool isBusy() const;

public slots:
    SessionTab* newTab();
    void closeTab(int index);

private:
    void renameTab(int index);
    void createMenus();

    QTabWidget* m_tabs = nullptr;
    int m_tabCounter = 0;
};

} // namespace iat
