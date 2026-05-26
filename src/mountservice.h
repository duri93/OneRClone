#pragma once

#include "settingsmanager.h"

#include <QObject>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QTimer>

// ---------------------------------------------------------------------------
// ServiceStatus
// ---------------------------------------------------------------------------
enum class ServiceStatus {
    Stopped,
    Starting,
    Running,
    Stopping,
    Errored
};

QString statusToString(ServiceStatus s);

// ---------------------------------------------------------------------------
// MountService
// One instance per service entry.  Lives on the main thread; QProcess
// communicates back via Qt signals (no extra threads needed).
// ---------------------------------------------------------------------------
class MountService : public QObject
{
    Q_OBJECT

public:
    explicit MountService(const ServiceSpec& spec,
                          const SharedSettings& shared,
                          QObject* parent = nullptr);
    ~MountService() override;

    // Accessors
    const QString&  id()     const { return m_spec.id; }
    const QString&  name()   const { return m_spec.name; }
    const QStringList& outputBuffer() const {return m_outputBuffer; }
    ServiceStatus   status() const { return m_status; }

    // Update spec (allowed only while stopped)
    void updateSpec(const ServiceSpec& spec);

    // Update shared settings (used when the user saves settings)
    void updateShared(const SharedSettings& shared);

public slots:
    void start();
    void stop();

signals:
    void statusChanged(const QString& serviceId, ServiceStatus newStatus);
    void outputLine(const QString& serviceId, const QString& line);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setStatus(ServiceStatus s);
    void processLine(const QString& line);

    ServiceSpec    m_spec;
    SharedSettings m_shared;
    ServiceStatus  m_status = ServiceStatus::Stopped;

    QTimer*            m_killTimer;
    QProcess           m_process;
    QStringList        m_outputBuffer;
    QRegularExpression m_errorRegex;
};
