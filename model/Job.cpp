#include "Job.h"

#include "../model/Config.h"
#include <QUuid>
#include <QTimer>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static std::atomic<int> s_ctrlSuppressCount{0};

Job::Job(SharedSettings* shared, QObject* parent)
    : QObject(parent), m_shared(shared){

    m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_name = "New job";

    // Wire up process signals
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &Job::onReadyRead);
    connect(&m_process, &QProcess::readyReadStandardError, this, &Job::onReadyRead);
    connect(&m_process, &QProcess::errorOccurred, this, &Job::onProcessError);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Job::onProcessFinished);

    // Merge stderr into stdout channel for unified output display
    // m_process.setProcessChannelMode(QProcess::MergedChannels);
}
Job::~Job()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_status = JobStatus::Stopping;
        m_process.kill();
        // m_process.waitForFinished(3000);
    }
}

const QString Job::statusString() const{
    switch(status()){
    case JobStatus::Stopped:
        return "Stopped";
    case JobStatus::Starting:
        return "Starting";
    case JobStatus::Running:
        return "Running";
    case JobStatus::Errored:
        return "Errored";
    case JobStatus::Stopping:
        return "Stopping";
    case JobStatus::Success:
        return "Success";
    }

    return "Unknown";
}


// ---------------------------------------------------------------------------
// start / stop
// ---------------------------------------------------------------------------
void Job::toggle(bool swapSides){
    if(m_status == JobStatus::Starting || m_status == JobStatus::Running){
        stop();
    }else{
        start(swapSides);
    }
}
void Job::start(bool swapSides)
{
    if (m_status != JobStatus::Stopped && m_status != JobStatus::Errored) {
        return;
    }

    setStatus(JobStatus::Starting);

    QStringList args;

    if(m_type == "mount"){
        args << "mount" << m_remote << m_local;
        if (m_readOnly) args << "--read-only";
        args << "--vfs-cache-mode"              << m_shared->cacheMode();
        args << "--vfs-cache-max-size"          << QString::number(m_shared->cacheMaxSize())       + "G";
        args << "--vfs-cache-min-free-space"    << QString::number(m_shared->cacheMinFreeSpace())  + "G";
        args << "--vfs-cache-max-age"           << QString::number(m_shared->cacheMaxAge())        + "h";
        args << "--vfs-read-chunk-size"         << QString::number(m_shared->readChunkSize())      + "M";
        args << "--vfs-read-chunk-size-limit"   << QString::number(m_shared->readChunkSizeLimit()) + "M";
    }else{
        args << m_type;
        if(swapSides){
            args << m_remote << m_local;
        }else{
            args << m_local << m_remote;
        }
        args << "--progress" << "--delete-before";
    }

    args << "--buffer-size"                 << QString::number(m_shared->bufferSize())         + "M";
    args << "--transfers"                   << QString::number(m_shared->transfers());
    args << "--checkers"                    << QString::number(m_shared->checkers());
    if (m_shared->links()) args << "--links";

    m_output.clear();
    m_output.append(args.join(' '));

    m_process.setProgram(m_shared->rclonePath());
    m_process.setArguments(args);
    m_process.start();

    // QProcess::started is not connected here; we transition to Running on
    // first stdout output to avoid false positives while rclone initialises.
}

void Job::stop()
{
    if (m_status == JobStatus::Stopped) return;
    setStatus(JobStatus::Stopping);

#ifdef Q_OS_WIN
    if (AttachConsole(m_process.processId())) {
        if (s_ctrlSuppressCount.fetch_add(1) == 0){
            SetConsoleCtrlHandler(nullptr, TRUE);   // suppress it in our process
        }

        GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
        FreeConsole();

        QTimer::singleShot(3000, this, [this]() {
            if (--s_ctrlSuppressCount == 0){
                SetConsoleCtrlHandler(nullptr, FALSE);  // restore
            }
            if (m_process.state() != QProcess::NotRunning)
                m_process.kill();  // fallback
        });

    } else {
        m_process.kill();
    }
#else
    m_process.kill();
#endif
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------
void Job::onReadyRead()
{
    // Read all available data line by line
    while (m_process.canReadLine()) {
        QString line = QString::fromUtf8(m_process.readLine()).trimmed();
        if (!line.isEmpty()) {
            processLine(line);
        }
    }
}
void Job::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    if (m_status != JobStatus::Stopping) {
        if(error == QProcess::FailedToStart){
            emit outputLine(m_id, QString("[ERROR] Failed to start: %1").arg(m_process.errorString()));
        }else{
            emit outputLine(m_id, QString("[ERROR] Process error: %1").arg(m_process.errorString()));
        }
        setStatus(JobStatus::Errored);
    }
}
void Job::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    // m_killTimer->stop();

    // Drain any remaining output
    onReadyRead();

    if (m_status == JobStatus::Errored) {
        // Process was stopped because of an error match — keep Errored status
        setStatus(JobStatus::Errored);
    } else if(m_status == JobStatus::Stopping){
        setStatus(JobStatus::Stopped);
    } else if (exitCode != 0 && exitCode != 62097) {
        emit outputLine(m_id, QString("[ERROR] Process exited with code %1").arg(exitCode));
        setStatus(JobStatus::Errored);
    } else {
        setStatus(JobStatus::Success);
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
void Job::setStatus(JobStatus s)
{
    if (m_status == s) return;
    m_status = s;
    emit statusChanged(m_id, s);
}
void Job::processLine(const QString& line)
{
    m_output.append(line);
    if(m_output.size() > Config::MAX_OUTPUT_LINES){
        m_output.removeFirst();
    }

    emit outputLine(m_id, line);

    // Transition from Starting -> Running on first real output
    if (m_status == JobStatus::Starting) {
        if (m_type == "mount") {
            // mount signals readiness with a specific string
            if (line.contains(Config::DEFAULT_START_STRING))
                setStatus(JobStatus::Running);
        } else {
            // sync/copy: any output means the process is alive and running
            setStatus(JobStatus::Running);
        }
    }

    // parse progress for sync/copy commands
    if (m_type != "mount")
        tryParseProgress(line);

    // Check for error pattern
    if (Config::ERROR_REGEX.match(line).hasMatch()) {
        setStatus(JobStatus::Errored);
        m_process.terminate();
        // m_killTimer->start(5000);
        return;
    }
}
void Job::tryParseProgress(const QString& line)
{
    // Matches: "Transferred: 123.45 MiB / 1.23 GiB, 10%, 1.23 MiB/s, ETA 1m23s"
    static const QRegularExpression re(
        R"(Transferred:\s+([\d.]+ \S+ \/ [\d.]+ \S+),\s+(\d+)%,\s+([\S]+ \S+\/s),\s+ETA\s+(\S+))");

    QRegularExpressionMatch m = re.match(line);
    if (!m.hasMatch()) return;

    // TODO: recognize as number and format number of significant digits?
    // TODO: limit refresh rate?
    m_progress.bytes   = m.captured(1);   // e.g. "123.45 MiB"
    m_progress.percent = m.captured(2).toInt();
    m_progress.speed   = m.captured(3);   // e.g. "1.23 MiB/s"
    m_progress.eta     = m.captured(4);   // e.g. "1m23s"

    emit progressUpdated(m_id, m_progress);
}

void Job::fromJson(const QJsonValue& json){
    this->m_id        = json["id"].toString();
    this->m_name      = json["name"].toString();
    this->m_type      = json["type"].toString();
    this->m_local     = json["local"].toString();
    this->m_remote    = json["remote"].toString();
    this->m_autostart = json["autostart"].toBool(false);
    this->m_readOnly  = json["readOnly"].toBool(false);

    if (this->m_id.isEmpty()) {
        this->m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
}
const QJsonObject Job::toJson() const{
    QJsonObject o;
    o["id"]        = this->m_id;
    o["name"]      = this->m_name;
    o["type"]      = this->m_type;
    o["local"]     = this->m_local;
    o["remote"]    = this->m_remote;
    o["autostart"] = this->m_autostart;
    o["readOnly"]  = this->m_readOnly;
    return o;
}

