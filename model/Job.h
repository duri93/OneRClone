#pragma once

#include "../model/SharedSettings.h"

#include <QObject>
#include <QProcess>
#include <QString>

enum class JobStatus{
    Stopped,
    Starting,
    Running,
    Success,
    Stopping,
    Errored
};

struct JobProgress{
    QString bytes = "";
    QString speed = "";
    int percent = 0;
    QString eta = "";
};

class Job : public QObject
{
    Q_OBJECT

public:
    explicit Job(SharedSettings* shared, QObject* parent = nullptr);
    ~Job() override;

    // Property accessors
    const QString id()       const { return m_id; }
    const QString name()     const { return m_name; }
    const QString type()     const { return m_type; }
    const QString local()     const {return m_local;     };
    const QString remote()    const {return m_remote;    };
    const bool    autostart() const {return m_autostart; };
    const bool    readOnly()  const {return m_readOnly;  };

    void setId       (QString newId    ){ m_id        = std::move(newId);        emit specsChanged(); }
    void setName     (QString newName  ){ m_name      = std::move(newName);      emit specsChanged(); }
    void setType     (QString newType  ){ m_type      = std::move(newType);      emit specsChanged(); }
    void setLocal    (QString newLocal ){ m_local     = std::move(newLocal);     emit specsChanged(); }
    void setRemote   (QString newRemote){ m_remote    = std::move(newRemote);    emit specsChanged(); }
    void setAutostart(bool newAutostart){ m_autostart = newAutostart; emit specsChanged(); }
    void setReadOnly (bool newReaOnly  ){ m_readOnly  = newReaOnly;   emit specsChanged(); }

    // Import / export
    const QJsonObject toJson() const;
    void fromJson(const QJsonValue& json);

    // Status accessors
    const QStringList   output()   const { return m_output; }
    const JobStatus     status()   const { return m_status; }
    const bool          active()   const { return m_status == JobStatus::Starting || m_status == JobStatus::Running; }
    const JobProgress   progress() const { return m_progress; }

    const QString       statusString() const;

    // Process handling
    void start(bool swapSides = false);
    void stop();

public slots:
    void toggle(bool swapSides = false);

signals:
    void specsChanged();
    void statusChanged(const QString& serviceId, JobStatus& newStatus);
    void progressUpdated(const QString& serviceId, JobProgress& newProgress);
    void outputLine(const QString& serviceId, const QString& line);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setStatus(JobStatus s);
    void processLine(const QString& line);
    void tryParseProgress(const QString& line);

    QString m_id;       // internal UUID, not shown in UI
    QString m_name    = "New job";
    QString m_type    = "sync";
    QString m_local   = "";
    QString m_remote  = "";
    bool m_autostart  = false;
    bool m_readOnly   = false;

    SharedSettings* m_shared;
    JobStatus       m_status = JobStatus::Stopped;
    JobProgress     m_progress;
    QProcess        m_process;
    QStringList     m_output;
};
