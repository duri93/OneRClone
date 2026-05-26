#pragma once

#include <QString>
#include <QVector>
#include <QJsonObject>

// ---------------------------------------------------------------------------
// ServiceSpec – plain data struct for one mount entry
// ---------------------------------------------------------------------------
struct ServiceSpec {
    QString id;       // internal UUID, not shown in UI
    QString name;
    QString local;
    QString remote;
    bool    automount = false;
    bool    readOnly = false;
    bool    links = true;
};

// ---------------------------------------------------------------------------
// SharedSettings – global rclone parameters
// ---------------------------------------------------------------------------
struct SharedSettings {
    QString rclonePath         = "C:\\RClone\\rclone.exe";
    QString cacheMode          = "full";
    int     cacheMaxSize       = 50;    // GB
    int     cacheMinFreeSpace  = 50;    // GB
    int     cacheMaxAge        = 24;    // hours
    int     readChunkSize      = 128;   // MB
    int     readChunkSizeLimit = 2048;  // MB
    int     bufferSize         = 256;   // MB
    int     transfers          = 16;
    int     checkers           = 16;
};

// ---------------------------------------------------------------------------
// SettingsManager – load / save settings.json next to the exe
// ---------------------------------------------------------------------------
class SettingsManager {
public:
    SettingsManager();

    // Returns false if the file didn't exist and defaults were written instead
    bool load();
    bool save() const;

    SharedSettings&        shared()         { return m_shared; }
    const SharedSettings&  shared()   const { return m_shared; }

    QVector<ServiceSpec>&        services()       { return m_services; }
    const QVector<ServiceSpec>&  services() const { return m_services; }

    // Helpers
    void addService(const ServiceSpec& spec);
    void updateService(const ServiceSpec& spec);   // matched by id
    void removeService(const QString& id);
    ServiceSpec* findService(const QString& id);

private:
    QString        m_filePath;
    SharedSettings m_shared;
    QVector<ServiceSpec> m_services;

    QJsonObject sharedToJson()  const;
    void        sharedFromJson(const QJsonObject& obj);
    QJsonObject specToJson(const ServiceSpec& spec) const;
    ServiceSpec specFromJson(const QJsonObject& obj) const;
};
