#include "MainWindow.h"

#include "ui_MainWindow.h"

#include "../model/Config.h"

#include <QFileDialog>
#include <QCloseEvent>
#include <QUuid>
#include <QListView>
#include <QSettings>
#include <QPainter>

#include <QRect>
#include <QSize>
#include <QScreen>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QString(Config::APP_NAME) + " " + Config::APP_VERSION);
    moveWindowToBottomRight();

    // ---- Load settings (generates defaults on first run) ----
    connect(&m_manager, &Manager::added, this, &MainWindow::onJobAdded);
    connect(&m_manager, &Manager::removed, this, &MainWindow::onJobRemoved);

    if (!m_manager.load()) {
        statusBar()->showMessage("Could not load settings file — using defaults.", Config::STATUS_DURATION);
    }

    // ---- Settings tab ----
    ui->advancedFrame->hide();
    loadSettingsToUi();
    connect(ui->advanced, &QCheckBox::checkStateChanged, this, &MainWindow::onSettingsAdvanced);
    connect(ui->settings_save, &QPushButton::clicked, this, &MainWindow::onSettingsSave);
    connect(ui->rcloneSelect,  &QToolButton::clicked, this, &MainWindow::onRcloneSelectClicked);

    // ---- List tab ----
    // populated when adding jobs
    connect(ui->listAdd, &QPushButton::clicked, this, &MainWindow::onAddJobClicked);

    // ---- Details tab ----
    ui->output->document()->setMaximumBlockCount(Config::MAX_OUTPUT_LINES);

    connect(ui->details_save,   &QPushButton::clicked, this, &MainWindow::onDetailsSave);
    connect(ui->details_delete, &QPushButton::clicked, this, &MainWindow::onDetailsDelete);
    connect(ui->localSelect,    &QToolButton::clicked, this, &MainWindow::onLocalSelectClicked);

    // Start on the list tab
    clearDetails();
    openDetails(nullptr);
    ui->tabWidget->setCurrentWidget(ui->tabList);

    // setup tray
    setupTray();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    hide();               // hide window, keep app running
    event->ignore();      // don't propagate the close
}

// ---------------------------------------------------------------------------
// Settings tab
// ---------------------------------------------------------------------------
void MainWindow::loadSettingsToUi()
{
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    bool isRegistered = reg.contains(Config::APP_ID);

    const SharedSettings* s = m_manager.shared();
    ui->rclone            ->setText   (s->rclonePath());
    ui->advanced          ->setChecked(s->advanced());
    ui->bufferSize        ->setValue  (s->bufferSize());
    ui->cacheMaxSize      ->setValue  (s->cacheMaxSize());
    ui->cacheMinFreeSpace ->setValue  (s->cacheMinFreeSpace());
    ui->cacheMaxAge       ->setValue  (s->cacheMaxAge());
    ui->readChunkSize     ->setValue  (s->readChunkSize());
    ui->readChunkSizeLimit->setValue  (s->readChunkSizeLimit());
    ui->transfers         ->setValue  (s->transfers());
    ui->checkers          ->setValue  (s->checkers());
    ui->links             ->setChecked(s->links());
    ui->startupLaunch     ->setChecked(isRegistered);

    // cacheMode combobox: find matching text
    int idx = ui->cacheMode->findText(s->cacheMode());
    if (idx >= 0) ui->cacheMode->setCurrentIndex(idx);
}

void MainWindow::saveUiToSettings()
{
    SharedSettings* s = m_manager.shared();
    s->setRclonePath(ui->rclone->text());
    s->setAdvanced(ui->advanced->isChecked());
    s->setCacheMode(ui->cacheMode->currentText());
    s->setCacheMaxSize(ui->cacheMaxSize->value());
    s->setCacheMinFreeSpace(ui->cacheMinFreeSpace->value());
    s->setCacheMaxAge(ui->cacheMaxAge->value());
    s->setReadChunkSize(ui->readChunkSize->value());
    s->setReadChunkSizeLimit(ui->readChunkSizeLimit->value());
    s->setBufferSize(ui->bufferSize->value());
    s->setTransfers(ui->transfers->value());
    s->setCheckers(ui->checkers->value());
    s->setLinks(ui->links->isChecked());

    // register or unregister startup
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if(ui->startupLaunch->isChecked()){
        reg.setValue(Config::APP_ID, QCoreApplication::applicationFilePath().replace('/', '\\') + " --tray");
    }else{
        reg.remove(Config::APP_ID);
    }
}

void MainWindow::onSettingsSave()
{
    saveUiToSettings();

    if(m_manager.save()){
        statusBar()->showMessage("Settings saved.", Config::STATUS_DURATION);
    }else{
        statusBar()->showMessage("Error saving settings.", Config::STATUS_DURATION);
    }
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
void MainWindow::onSettingsAdvanced(){
    if(ui->advanced->isChecked()){
        ui->advancedFrame->show();
    }else{
        ui->advancedFrame->hide();
    }
}

// ---------------------------------------------------------------------------
// Jobs list tab
// ---------------------------------------------------------------------------
void MainWindow::onAddJobClicked()
{
    Job* job = new Job(m_manager.shared());
    m_manager.addJob(job);
    openDetails(job);
}

// ---------------------------------------------------------------------------
// Job details tab
// ---------------------------------------------------------------------------
void MainWindow::openDetails(Job* job)
{
    // enable / disable panel
    bool validJob = (bool) job;

    ui->name          ->setEnabled(validJob);
    ui->type          ->setEnabled(validJob);
    ui->local         ->setEnabled(validJob);
    ui->remote        ->setEnabled(validJob);
    ui->autostart     ->setEnabled(validJob);
    ui->readOnly      ->setEnabled(validJob);
    ui->details_save  ->setEnabled(validJob);
    ui->details_delete->setEnabled(validJob);

    if(!validJob) return;

    // save current job
    m_currentJobDetails = job;

    // update ui
    ui->name->setText(job->name());
    ui->type->setCurrentIndex(ui->type->findText(job->type()));
    ui->local->setText(job->local());
    ui->remote->setText(job->remote());
    ui->autostart->setChecked(job->autostart());
    ui->readOnly->setChecked(job->readOnly());

    // populate output log
    ui->output->clear();
    for(const QString& line : job->output()){
        ui->output->append(line);
    }

    // show details tab
    ui->tabWidget->setCurrentWidget(ui->tabDetails);
}

void MainWindow::clearDetails()
{
    m_currentJobDetails = nullptr;

    ui->name->clear();
    ui->local->clear();
    ui->remote->clear();
    ui->autostart->setChecked(false);
    ui->readOnly->setChecked(false);
    ui->output->clear();
}

void MainWindow::onDetailsSave()
{
    if(!m_currentJobDetails) return;
    Job* job = m_currentJobDetails;

    job->setName(ui->name->text());
    job->setType(ui->type->currentText());
    job->setLocal(ui->local->text());
    job->setRemote(ui->remote->text());
    job->setAutostart(ui->autostart->isChecked());
    job->setReadOnly(ui->readOnly->isChecked());

    if(m_manager.save()){
        statusBar()->showMessage("Job saved.", Config::STATUS_DURATION);
        ui->tabWidget->setCurrentWidget(ui->tabList);
    }else{
        statusBar()->showMessage("Error saving job.", Config::STATUS_DURATION);
    }
}

void MainWindow::onDetailsDelete()
{
    if (!m_currentJobDetails) return;

    Job* toRemove = m_currentJobDetails;
    m_currentJobDetails = nullptr;   // clear before deletion
    m_manager.removeJob(toRemove);

    if(m_manager.save()){
        statusBar()->showMessage("Job removed.", Config::STATUS_DURATION);

        clearDetails();
        openDetails(nullptr);
        ui->tabWidget->setCurrentWidget(ui->tabList);
    }else{
        statusBar()->showMessage("Error removing job.", Config::STATUS_DURATION);
    }

}

void MainWindow::onLocalSelectClicked()
{
    QString path = QFileDialog::getExistingDirectory(
        this, "Select local folder or mount point", ui->local->text());
    if (!path.isEmpty()) {
        ui->local->setText(QDir::toNativeSeparators(path));
    }
}

// ---------------------------------------------------------------------------
// Model events
// ---------------------------------------------------------------------------
void MainWindow::onJobOutputLine(const QString& id, const QString& line)
{
    // Only append to the output widget if the right service is in the details tab
    if (!m_currentJobDetails || id != m_currentJobDetails->id()) return;

    ui->output->append(line);

}
void MainWindow::onJobAdded(Job* job){
    JobWidget* w = new JobWidget(job);

    connect(w, &JobWidget::openDetailsRequested, this, [this](const QString& id) {
        openDetails(m_manager.getJob(id));
    });

    connect(job, &Job::outputLine,    this, &MainWindow::onJobOutputLine);
    m_jobWidgets.append(w);
    ui->jobsList->layout()->addWidget(w);
}

void MainWindow::onJobRemoved(const QString& jobId){
    for (int i = 0; i < m_jobWidgets.size(); ++i) {
        if (m_jobWidgets[i]->job()->id() == jobId) {
            delete m_jobWidgets[i];
            m_jobWidgets.removeAt(i);
            break;
        }
    }
};

// ---------------------------------------------------------------------------
// Tray icon
// ---------------------------------------------------------------------------
void MainWindow::setupTray()
{
    m_trayIcon = new QSystemTrayIcon(this);

    // Placeholder icon — replace with your actual app icon
    m_trayIcon->setIcon(QIcon(":/resources/favicon.svg"));
    m_trayIcon->setToolTip(Config::APP_NAME);

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

void MainWindow::onTrayMenuAboutToShow()
{
    // Remove all actions between the first separator and the last separator
    // (i.e. the dynamically added service actions from last time)
    QList<QAction*> actions = m_trayMenu->actions();
    QAction* lastSep  = nullptr;
    bool inside = false;
    for (QAction*& a : actions) {
        if (a->isSeparator()) {
            lastSep = a;
            inside = !inside;
        }else if (inside){
            m_trayMenu->removeAction(a);
            delete a;
        }
    }

    // Re-insert current service actions before lastSep
    for(JobWidget*& jw : m_jobWidgets){
        Job* job = jw->job();
        QPixmap icon = QPixmap(jw->icon());

        QAction* act = new QAction(icon, job->name(), m_trayMenu);

        // Toggle on click
        connect(act, &QAction::triggered, this, [job]() {
            if (job->status() == JobStatus::Running ||
                job->status() == JobStatus::Starting)
                job->stop();
            else
                job->start();
        });

        m_trayMenu->insertAction(lastSep, act);
    }
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
    }
}

void MainWindow::moveWindowToBottomRight(){
    // position window in bottom-right corner
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect available = screen->availableGeometry(); // excludes taskbar
    QSize  size     = frameSize().isEmpty() ? sizeHint() : frameSize();

    int x = available.right()  - size.width()  - 6;
    int y = available.bottom() - size.height() - 38;
    move(x, y);
}

void MainWindow::activate(){
    show();
    setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
}