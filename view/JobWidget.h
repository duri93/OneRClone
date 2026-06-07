#pragma once

#include "../model/Job.h"

#include <QWidget>

namespace Ui {
class JobWidget;
}

class JobWidget : public QWidget
{
    Q_OBJECT

public:
    explicit JobWidget(Job* job);
    ~JobWidget() override;

    Job*    job () { return m_job; };
    const Job*    job () const { return m_job; };

    // Called from MainWindow slots
    void onSpecChange();
    void onStatusChange();
    void onProgress();
    void onWarning();

    const QPixmap getJobIcon() const;
    const QPixmap getStatusIcon() const;

signals:
    void openDetailsRequested(const QString& id);

private:
    Ui::JobWidget* ui;
    Job*       m_job;
    QPoint m_dragStartPos;

    void setProgressVisibility();
protected:
    // TODO only from icon
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event)  override;

    bool eventFilter(QObject* watched, QEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
};
