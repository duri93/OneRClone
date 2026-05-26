#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "servicelistdelegate.h"
#include "mountservice.h"

#include <QFileDialog>
#include <QCloseEvent>
#include <QUuid>
#include <QListView>
#include <QSettings>
#include <QPainter>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ---- Load settings (generates defaults on first run) ----
    m_settings.load();

    // ---- Populate service model from loaded specs ----
    for (const ServiceSpec& spec : m_settings.services()) {
        m_model.addService(spec, m_settings.shared());
    }

    // ---- Settings tab ----
    loadSettingsToUi();
    connect(ui->settings_save, &QPushButton::clicked, this, &MainWindow::onSettingsSave);
    connect(ui->rcloneSelect,  &QToolButton::clicked, this, &MainWindow::onRcloneSelectClicked);

    // ---- List tab: create QListView ----
    ui->listView->setModel(&m_model);

    auto* delegate = new ServiceListDelegate(ui->listView);
    ui->listView->setItemDelegate(delegate);
    ui->listView->setMouseTracking(true);

    connect(ui->listView, &QAbstractItemView::entered, [delegate](const QModelIndex&) { Q_UNUSED(delegate) });
    ui->listView->viewport()->installEventFilter(delegate);

    connect(ui->listAdd, &QPushButton::clicked, this, &MainWindow::onAddMount);
    connect(ui->listView, &QListView::doubleClicked, this, &MainWindow::onListDoubleClicked);
    connect(delegate, &ServiceListDelegate::toggleRequested, this, &MainWindow::onToggleRequested);

    // ---- Details tab ----
    connect(ui->details_save,   &QPushButton::clicked, this, &MainWindow::onDetailsSave);
    connect(ui->details_delete, &QPushButton::clicked, this, &MainWindow::onDetailsDelete);
    connect(ui->localSelect,    &QToolButton::clicked, this, &MainWindow::onLocalSelectClicked);

    // ---- Model events ----
    connect(&m_model, &ServiceListModel::serviceStatusChanged, this, &MainWindow::onServiceStatusChanged);
    connect(&m_model, &ServiceListModel::serviceOutputLine, this, &MainWindow::onServiceOutputLine);

    // Start on the list tab
    ui->tabWidget->setCurrentWidget(ui->tabList);
    clearDetails();

    // Auto-start services
    for (const ServiceSpec& spec : m_settings.services()) {
        if (spec.automount) {
            MountService* svc = m_model.serviceById(spec.id);
            if (svc) svc->start();
        }
    }

    // setup tray
    setupTray();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    m_settings.save();    // save settings
    hide();               // hide window, keep app running
    event->ignore();      // don't propagate the close
}

// ---------------------------------------------------------------------------
// Settings tab
// ---------------------------------------------------------------------------
void MainWindow::loadSettingsToUi()
{
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    bool isRegistered = reg.contains("SRCMG");

    const SharedSettings& s = m_settings.shared();
    ui->rclone->setText(s.rclonePath);
    ui->bufferSize->setValue(s.bufferSize);
    ui->cacheMaxSize->setValue(s.cacheMaxSize);
    ui->cacheMinFreeSpace->setValue(s.cacheMinFreeSpace);
    ui->cacheMaxAge->setValue(s.cacheMaxAge);
    ui->readChunkSize->setValue(s.readChunkSize);
    ui->readChunkSizeLimit->setValue(s.readChunkSizeLimit);
    ui->transfers->setValue(s.transfers);
    ui->checkers->setValue(s.checkers);
    ui->startupLaunch->setChecked(isRegistered);

    // cacheMode combobox: find matching text
    int idx = ui->cacheMode->findText(s.cacheMode);
    if (idx >= 0) ui->cacheMode->setCurrentIndex(idx);
}

void MainWindow::saveUiToSettings()
{
    SharedSettings& s = m_settings.shared();
    s.rclonePath         = ui->rclone->text();
    s.cacheMode          = ui->cacheMode->currentText();
    s.cacheMaxSize       = ui->cacheMaxSize->value();
    s.cacheMinFreeSpace  = ui->cacheMinFreeSpace->value();
    s.cacheMaxAge        = ui->cacheMaxAge->value();
    s.readChunkSize      = ui->readChunkSize->value();
    s.readChunkSizeLimit = ui->readChunkSizeLimit->value();
    s.bufferSize         = ui->bufferSize->value();
    s.transfers          = ui->transfers->value();
    s.checkers           = ui->checkers->value();

    // register or unregister startup
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if(ui->startupLaunch->isChecked()){
        reg.setValue("SRCMG", QCoreApplication::applicationFilePath().replace('/', '\\') + " --tray");
    }else{
        reg.remove("SRCMG");
    }
}

void MainWindow::onSettingsSave()
{
    saveUiToSettings();
    m_settings.save();

    // Propagate updated shared settings to all services
    m_model.updateAllShared(m_settings.shared());

    statusBar()->showMessage("Settings saved.", 3000);
}

void MainWindow::onRcloneSelectClicked()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Select rclone.exe", ui->rclone->text(),
        "Executable (*.exe);;All files (*.*)");
    if (!path.isEmpty()) {
        ui->rclone->setText(QDir::toNativeSeparators(path));
    }
}

// ---------------------------------------------------------------------------
// Mounts list tab
// ---------------------------------------------------------------------------
void MainWindow::onAddMount()
{
    ServiceSpec spec;
    spec.id   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    spec.name = "New Mount";

    m_settings.addService(spec);

    spec = *m_settings.findService(spec.id);
    m_model.addService(spec, m_settings.shared());

    openDetails(spec.id);
}

void MainWindow::onListDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    QString id = index.data(ServiceRole::Id).toString();
    openDetails(id);
}

void MainWindow::onToggleRequested(const QModelIndex& index)
{
    if (!index.isValid()) return;
    QString id  = index.data(ServiceRole::Id).toString();
    auto*   svc = m_model.serviceById(id);
    if (!svc) return;

    if (svc->status() == ServiceStatus::Running ||
        svc->status() == ServiceStatus::Stopping ||
        svc->status() == ServiceStatus::Starting) {
        svc->stop();
    } else {
        svc->start();
    }
}

// ---------------------------------------------------------------------------
// Mount details tab
// ---------------------------------------------------------------------------
void MainWindow::openDetails(const QString& serviceId)
{
    ServiceSpec* spec = m_settings.findService(serviceId);
    MountService* srvc = m_model.serviceById(serviceId);

    if (!spec || !srvc) return;

    m_currentDetailId = serviceId;

    ui->name->setText(spec->name);
    ui->local->setText(spec->local);
    ui->remote->setText(spec->remote);
    ui->automount->setChecked(spec->automount);
    ui->readOnly->setChecked(spec->readOnly);
    ui->links->setChecked(spec->links);

    // Clear output log and show what we have buffered (none yet for new
    // services; this is a live view populated via onServiceOutputLine)
    ui->output->clear();
    for(const QString& line : srvc->outputBuffer()){
        ui->output->append(line);
    }

    ui->tabWidget->setCurrentWidget(ui->tabDetails);
}

void MainWindow::clearDetails()
{
    m_currentDetailId.clear();
    ui->name->clear();
    ui->local->clear();
    ui->remote->clear();
    ui->automount->setChecked(false);
    ui->readOnly->setChecked(false);
    ui->links->setChecked(true);
    ui->output->clear();
}

QString MainWindow::currentDetailId() const
{
    return m_currentDetailId;
}

void MainWindow::onDetailsSave()
{
    if (m_currentDetailId.isEmpty()) return;

    ServiceSpec* spec = m_settings.findService(m_currentDetailId);
    if (!spec) return;

    spec->name   = ui->name->text();
    spec->local  = ui->local->text();
    spec->remote = ui->remote->text();
    spec->automount = ui->automount->isChecked();
    spec->readOnly = ui->readOnly->isChecked();
    spec->links  = ui->links->isChecked();

    m_settings.updateService(*spec);
    m_model.updateSpec(m_currentDetailId, *spec);
    m_settings.save();

    ui->tabWidget->setCurrentWidget(ui->tabList);

    statusBar()->showMessage("Mount saved.", 3000);
}

void MainWindow::onDetailsDelete()
{
    if (m_currentDetailId.isEmpty()) return;

    m_settings.removeService(m_currentDetailId);
    m_model.removeService(m_currentDetailId);
    m_settings.save();

    clearDetails();
    ui->tabWidget->setCurrentWidget(ui->tabList);

    statusBar()->showMessage("Mount removed.", 3000);
}

void MainWindow::onLocalSelectClicked()
{
    QString path = QFileDialog::getExistingDirectory(
        this, "Select local mount point", ui->local->text());
    if (!path.isEmpty()) {
        ui->local->setText(QDir::toNativeSeparators(path));
    }
}

// ---------------------------------------------------------------------------
// Model events
// ---------------------------------------------------------------------------
void MainWindow::onServiceStatusChanged(const QString& id, ServiceStatus status)
{
    // update the status bar
    statusBar()->showMessage(
        QString("Mount \"%1\": %2")
            .arg(m_model.serviceById(id) ? m_model.serviceById(id)->name() : id, statusToString(status)),
        5000);

    // redraw line to update button
    int row = m_model.rowForId(id);
    if (row >= 0) {
        QModelIndex idx = m_model.index(row);
        // Find the listView by name since it's created dynamically

        if (ui->listView) {
            ui->listView->update(idx);  // repaints just that row
        }
    }
}

void MainWindow::onServiceOutputLine(const QString& id, const QString& line)
{
    // Only append to the output widget if the right service is in the details tab
    if (id != m_currentDetailId) return;

    if(ui->output->document()->blockCount() >= 2000){
        ui->output->document()->clear();
    }

    ui->output->append(line);

}

// ---------------------------------------------------------------------------
// Tray icon
// ---------------------------------------------------------------------------
void MainWindow::setupTray()
{
    m_trayIcon = new QSystemTrayIcon(this);

    // Placeholder icon — replace with your actual app icon
    m_trayIcon->setIcon(QIcon(":/assets/favicon.svg"));
    m_trayIcon->setToolTip("Simple RClone Mount GUI");

    m_trayMenu  = new QMenu(this);
    m_trayOpen  = m_trayMenu->addAction("Open");
    m_trayMenu->addSeparator();
    // Service actions are inserted here dynamically (see onTrayMenuAboutToShow)
    m_trayMenu->addSeparator();
    m_trayClose = m_trayMenu->addAction("Quit");

    connect(m_trayOpen,  &QAction::triggered,         this, &MainWindow::show);
    connect(m_trayClose, &QAction::triggered,         qApp, &QApplication::quit);
    connect(m_trayMenu,  &QMenu::aboutToShow,         this, &MainWindow::onTrayMenuAboutToShow);
    connect(m_trayIcon,  &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();
}
QIcon MainWindow::dotIcon(QColor color)
{
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(2, 2, 12, 12);
    return QIcon(pm);
}
void MainWindow::onTrayMenuAboutToShow()
{
    // Remove all actions between the first separator and the last separator
    // (i.e. the dynamically added service actions from last time)
    QList<QAction*> actions = m_trayMenu->actions();
    QAction* lastSep  = nullptr;
    bool inside = false;
    for (QAction* a : actions) {
        if (a->isSeparator()) {
            lastSep = a;
            inside = !inside;
        }else if (inside){
            m_trayMenu->removeAction(a);
            delete a;
        }
    }

    // Re-insert current service actions before lastSep
    for (const ServiceSpec& spec : m_settings.services()) {
        MountService* svc = m_model.serviceById(spec.id);
        if (!svc) continue;

        QColor dot;
        switch (svc->status()) {
        case ServiceStatus::Stopped:  dot = QColor(0xAA, 0xAA, 0xAA); break;
        case ServiceStatus::Starting: dot = QColor(0xFF, 0xCC, 0x00); break;
        case ServiceStatus::Stopping: dot = QColor(0xFF, 0xCC, 0x00); break;
        case ServiceStatus::Running:  dot = QColor(0x22, 0xBB, 0x22); break;
        case ServiceStatus::Errored:  dot = QColor(0xDD, 0x22, 0x22); break;
        }

        QAction* act = new QAction(dotIcon(dot), spec.name, m_trayMenu);

        // Toggle on click
        connect(act, &QAction::triggered, this, [this, id = spec.id]() {
            MountService* s = m_model.serviceById(id);
            if (!s) return;
            if (s->status() == ServiceStatus::Running ||
                s->status() == ServiceStatus::Starting)
                s->stop();
            else
                s->start();
        });

        m_trayMenu->insertAction(lastSep, act);
    }
}


void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        show();
        raise();
        activateWindow();
    }
}



