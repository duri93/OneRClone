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

    const QString jobIcon() const;
    const QString statusIcon() const;
    Job*    job () { return m_job; };
    const Job*    job () const { return m_job; };

    // Called from MainWindow slots
    void onSpecChange();
    void onStatusChange();
    void onProgress();
    void setProgressVisibility();

signals:
    void openDetailsRequested(const QString& id);

private:
    Ui::JobWidget* ui;
    Job*       m_job;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
};
