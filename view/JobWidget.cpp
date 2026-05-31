#include "JobWidget.h"
#include "view/ui_JobWidget.h"

#include <QApplication>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QFont>

// CONSTRUCT
JobWidget::JobWidget(Job *job) : QWidget(nullptr), ui(new Ui::JobWidget) {
    ui->setupUi(this);
    m_job = job;

    // populate widget with data
    onSpecChange();
    onStatusChange();

    // enable mouse tracking and intercept double-click
    setMouseTracking(true);
    installEventFilter(this);

    // connect events
    connect(job, &Job::specsChanged,    this, &JobWidget::onSpecChange);
    connect(job, &Job::statusChanged,   this, &JobWidget::onStatusChange);
    connect(job, &Job::progressUpdated, this, &JobWidget::onProgress);

    connect(ui->button1, &QPushButton::clicked, this, [this]() {
        m_job->toggle(false);
    });
    connect(ui->button2, &QPushButton::clicked, this, [this]() {
        m_job->toggle(true);
    });
}
JobWidget::~JobWidget() {
    delete ui;
}
const QString JobWidget::icon() const{
    QString str = QString(":/resources/icons/%1_%2.svg").arg(m_job->type(), m_job->statusString());
    str = str.toLower();

    return str;
}

// MOUSE EVENTs
bool JobWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == this && event->type() == QEvent::MouseButtonDblClick) {
        auto* me = static_cast<QMouseEvent*>(event);
        // Only trigger if the click wasn't on a button
        if (!ui->button1->geometry().contains(me->pos()) &&
            !ui->button2->geometry().contains(me->pos()))
        {
            emit openDetailsRequested(m_job->id());
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
void JobWidget::enterEvent(QEnterEvent* event)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window,
                 QApplication::palette().color(QPalette::Base).lighter(115));
    setPalette(pal);
    QWidget::enterEvent(event);
}

void JobWidget::leaveEvent(QEvent* event)
{
    setAutoFillBackground(false);
    setPalette(QApplication::palette());
    QWidget::leaveEvent(event);
}

// OTHERS
void JobWidget::onSpecChange(){
    ui->name->setText(m_job->name());

    if(m_job->type() == "mount"){
        ui->button2->hide();
    }else{
        ui->button2->show();
    }

    bool validType = m_job->type() != "";
    ui->button1->setEnabled(validType);
    ui->button2->setEnabled(validType);

    ui->status->setFont(QFont("Arial", 8));
    ui->bytes->setFont(QFont("Arial", 8));
    ui->speed->setFont(QFont("Arial", 8));
    ui->percent->setFont(QFont("Arial", 8));
    ui->eta->setFont(QFont("Arial", 8));

    onStatusChange();
}
void JobWidget::onStatusChange(){
    bool active = m_job->active();

    // status label
    QString str = "[" + m_job->statusString() + "]";
    ui->status->setText(str);

    // status icon
    QPixmap px(icon());
    ui->icon->setPixmap(px.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // button labels
    if(m_job->type() == "mount"){
        ui->button1->setText(active ? "Stop" : "Mount");
    }else if(m_job->type() == "copy"){
        ui->button1->setText(active ? "Stop" : "Copy ▲");
        ui->button2->setText(active ? "Stop" : "Copy ▼");

        if(active){
            ui->button2->hide();
        }else{
            ui->button2->show();
        }
    }else if(m_job->type() == "sync"){
        ui->button1->setText(active ? "Stop" : "Sync ▲");
        ui->button2->setText(active ? "Stop" : "Sync ▼");

        if(active){
            ui->button2->hide();
        }else{
            ui->button2->show();
        }
    }

    // show progress if copy/sync is running
    setProgressVisibility();
}
void JobWidget::onProgress(){
    ui->bytes->setText(m_job->progress().bytes);
    ui->speed->setText(m_job->progress().speed);
    ui->percent->setText(QString::number(m_job->progress().percent) + "%");
    ui->eta->setText(m_job->progress().eta);

    ui->progress->setValue(m_job->progress().percent);
}
void JobWidget::setProgressVisibility(){
    bool active = m_job->active();

    if(m_job->type() != "mount" && active){
        ui->progress->show();
        ui->bytes->show();
        ui->speed->show();
        ui->percent->show();
        ui->eta->show();
    }else{
        ui->bytes->setText("");
        ui->speed->setText("");
        ui->percent->setText("");
        ui->eta->setText("");
        ui->progress->setValue(0);

        ui->progress->hide();
        ui->speed->hide();
        ui->bytes->hide();
        ui->percent->hide();
        ui->eta->hide();
    }

    adjustSize();
}

