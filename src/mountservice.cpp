#include "mountservice.h"
#include "appconfig.h"

#include <QTextStream>

// ---------------------------------------------------------------------------
// Free helper
// ---------------------------------------------------------------------------
QString statusToString(ServiceStatus s)
{
    switch (s) {
        case ServiceStatus::Stopped:  return "Stopped";
        case ServiceStatus::Starting: return "Starting";
        case ServiceStatus::Running:  return "Running";
        case ServiceStatus::Stopping: return "Stopping";
        case ServiceStatus::Errored:  return "Errored";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// MountService
// ---------------------------------------------------------------------------
MountService::MountService(const ServiceSpec& spec,
                           const SharedSettings& shared,
                           QObject* parent)
    : QObject(parent)
    , m_spec(spec)
    , m_shared(shared)
{
    m_errorRegex = QRegularExpression(AppConfig::DEFAULT_ERROR_REGEX);

    // Wire up process signals
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &MountService::onReadyRead);
    connect(&m_process, &QProcess::readyReadStandardError, this, &MountService::onReadyRead);
    connect(&m_process, &QProcess::errorOccurred, this, &MountService::onProcessError);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MountService::onProcessFinished);

    // Merge stderr into stdout channel for unified output display
    m_process.setProcessChannelMode(QProcess::MergedChannels);

    // setup QTimer waiting when closing the process
    m_killTimer = new QTimer(this);
    m_killTimer->setSingleShot(true);
    connect(m_killTimer, &QTimer::timeout, &m_process, &QProcess::kill);
}

MountService::~MountService()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_status = ServiceStatus::Stopping;
        m_process.kill();
        m_process.waitForFinished(3000);
    }
}

void MountService::updateSpec(const ServiceSpec& spec)
{
    // Q_ASSERT(m_status == ServiceStatus::Stopped);
    m_spec = spec;
}

void MountService::updateShared(const SharedSettings& shared)
{
    m_shared = shared;
}

// ---------------------------------------------------------------------------
// start / stop
// ---------------------------------------------------------------------------
void MountService::start()
{
    if (m_status != ServiceStatus::Stopped && m_status != ServiceStatus::Errored) {
        return;
    }

    setStatus(ServiceStatus::Starting);

    // Build command: program + args come from SettingsManager logic
    // We replicate the argument-building here via a temporary SettingsManager
    // call.  To avoid a circular dependency, MountService replicates the
    // argument list construction.

    QStringList args;
    args << "mount";
    args << m_spec.remote;
    args << m_spec.local;
    args << "--vfs-cache-mode"              << m_shared.cacheMode;
    args << "--vfs-cache-max-size"          << QString::number(m_shared.cacheMaxSize)      + "G";
    args << "--vfs-cache-min-free-space"    << QString::number(m_shared.cacheMinFreeSpace) + "G";
    args << "--vfs-cache-max-age"           << QString::number(m_shared.cacheMaxAge)       + "h";
    args << "--vfs-read-chunk-size"         << QString::number(m_shared.readChunkSize)     + "M";
    args << "--vfs-read-chunk-size-limit"   << QString::number(m_shared.readChunkSizeLimit)+ "M";
    args << "--buffer-size"                 << QString::number(m_shared.bufferSize)        + "M";
    args << "--transfers"                   << QString::number(m_shared.transfers);
    args << "--checkers"                    << QString::number(m_shared.checkers);
    if (m_spec.links) args << "--links";
    if (m_spec.readOnly) args << "--read-only";

    // -v makes rclone write progress/errors to stderr (merged above)
    //args << "-v";

    m_outputBuffer.clear();
    m_outputBuffer.append(args.join(' '));

    m_process.setProgram(m_shared.rclonePath);
    m_process.setArguments(args);
    m_process.start();

    // QProcess::started is not connected here; we transition to Running on
    // first stdout output to avoid false positives while rclone initialises.
}

void MountService::stop()
{
    if (m_status == ServiceStatus::Stopped) return;

    setStatus(ServiceStatus::Stopping);

    m_process.terminate();
    m_killTimer->start(5000);
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------
void MountService::onReadyRead()
{
    // Read all available data line by line
    while (m_process.canReadLine()) {
        QString line = QString::fromUtf8(m_process.readLine()).trimmed();
        if (!line.isEmpty()) {
            processLine(line);
        }
    }
}

void MountService::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    if (m_status != ServiceStatus::Stopping) {
        emit outputLine(m_spec.id,
            QString("[ERROR] Process error: %1").arg(m_process.errorString()));
        setStatus(ServiceStatus::Errored);
    }
}

void MountService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    m_killTimer->stop();

    // Drain any remaining output
    onReadyRead();

    if (m_status == ServiceStatus::Errored) {
        // Process was stopped because of an error match — keep Errored status
        setStatus(ServiceStatus::Errored);
    } else if (exitCode != 0 && exitCode != 62097) {
        emit outputLine(m_spec.id, QString("[ERROR] Process exited with code %1").arg(exitCode));
        setStatus(ServiceStatus::Errored);
    } else if (m_status == ServiceStatus::Stopping) {
        setStatus(ServiceStatus::Stopped);
    } else {
        setStatus(ServiceStatus::Stopped);
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
void MountService::setStatus(ServiceStatus s)
{
    if (m_status == s) return;
    m_status = s;
    emit statusChanged(m_spec.id, s);
}

void MountService::processLine(const QString& line)
{
    m_outputBuffer.append(line);
    if(m_outputBuffer.size() > 2000){
        m_outputBuffer.removeFirst();
    }

    emit outputLine(m_spec.id, line);

    // Transition from Starting -> Running on first real output
    if (m_status == ServiceStatus::Starting && line.contains(AppConfig::DEFAULT_START_STRING)) {
        setStatus(ServiceStatus::Running);
    }

    // Check for error pattern
    if (m_errorRegex.isValid() && m_errorRegex.match(line).hasMatch()) {
        setStatus(ServiceStatus::Errored);
        m_process.terminate();
        m_killTimer->start(5000);
    }
}
