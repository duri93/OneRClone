#pragma once

#include "../model/job.h"
#include "../model/Manager.h"
#include "../view/JobWidget.h"

#include <QMainWindow>
#include <QString>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ---------------------------------------------------------------------------
// MainWindow
// Wires the .ui widgets to SettingsManager and ServiceListModel.
// Follows a simple signals-&-slots architecture — no extra MVP layer.
// ---------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void activate();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Settings tab
    void onSettingsSave();
    void onRcloneSelectClicked();
    void onSettingsAdvanced();

    // Jobs list tab
    void onAddJobClicked();
    void onJobMoved(const QString& id, int newIndex);

    // Job details tab
    void onDetailsSave();
    void onDetailsDelete();
    void onLocalSelectClicked();

    // Job model events
    void onJobOutputLine(const QString& id, const QString& line);
    void onJobAdded(Job* job);
    void onJobRemoved(const QString& jobId);

    // Tray icon
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayMenuAboutToShow();    // rebuilds service list in menu

private:
    // Settings
    void loadSettingsToUi();
    void saveUiToSettings();

    // Job details
    void openDetails(Job* job);
    void clearDetails();

    // Window management
    void moveWindowToBottomRight();

    // Properties
    Ui::MainWindow*  ui;
    Manager  m_manager;

    Job*                m_currentJobDetails = nullptr;
    QVector<JobWidget*> m_jobWidgets;

private:
    // list
    JobWidget* findOrCreateJobWidget(Job* job);

    // general error checkers
    bool isRcloneInstalled();
    bool isWinFspInstalled();

    // tray icon
    void setupTray();

    QSystemTrayIcon* m_trayIcon    = nullptr;
    QMenu*           m_trayMenu    = nullptr;
    QAction*         m_trayOpen    = nullptr;
    QAction*         m_trayClose   = nullptr;
    // service actions are rebuilt dynamically in onTrayMenuAboutToShow
};
