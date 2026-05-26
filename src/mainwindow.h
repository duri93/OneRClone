#pragma once

#include "settingsmanager.h"
#include "servicelistmodel.h"

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

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Settings tab
    void onSettingsSave();
    void onRcloneSelectClicked();

    // Mounts list tab
    void onAddMount();
    void onListDoubleClicked(const QModelIndex& index);
    void onToggleRequested(const QModelIndex& index);

    // Mount details tab
    void onDetailsSave();
    void onDetailsDelete();
    void onLocalSelectClicked();

    // Service model events
    void onServiceStatusChanged(const QString& id, ServiceStatus status);
    void onServiceOutputLine(const QString& id, const QString& line);

    // Tray icon
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayMenuAboutToShow();    // rebuilds service list in menu

private:
    // Populate settings form from m_settings.shared()
    void loadSettingsToUi();
    // Read settings form into m_settings.shared()
    void saveUiToSettings();

    // Show a service's data in the details tab and switch to it
    void openDetails(const QString& serviceId);

    // Clear the details tab (no service selected)
    void clearDetails();

    // Return the id currently loaded in the details tab ("" if none)
    QString currentDetailId() const;

    Ui::MainWindow*  ui;
    SettingsManager  m_settings;
    ServiceListModel m_model;

    // Id of the service currently shown in the details tab
    QString m_currentDetailId;

// tray icon
private:
    void setupTray();
    QIcon dotIcon(QColor color);    // generates a small colored-dot icon

    QSystemTrayIcon* m_trayIcon    = nullptr;
    QMenu*           m_trayMenu    = nullptr;
    QAction*         m_trayOpen    = nullptr;
    QAction*         m_trayClose   = nullptr;
    // service actions are rebuilt dynamically in onTrayMenuAboutToShow
};
