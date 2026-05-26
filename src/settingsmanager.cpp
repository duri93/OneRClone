#include "settingsmanager.h"
#include "appconfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>

SettingsManager::SettingsManager()
{
    // Resolve path: same directory as the executable
    m_filePath = QDir(QCoreApplication::applicationDirPath())
                     .filePath(AppConfig::SETTINGS_FILENAME);

    // Apply compile-time defaults to shared settings
    m_shared.rclonePath         = AppConfig::DEFAULT_RCLONE_PATH;
    m_shared.cacheMode          = AppConfig::DEFAULT_CACHE_MODE;
    m_shared.cacheMaxSize       = AppConfig::DEFAULT_CACHE_MAX_SIZE;
    m_shared.cacheMinFreeSpace  = AppConfig::DEFAULT_CACHE_MIN_FREE_SPACE;
    m_shared.cacheMaxAge        = AppConfig::DEFAULT_CACHE_MAX_AGE;
    m_shared.readChunkSize      = AppConfig::DEFAULT_READ_CHUNK_SIZE;
    m_shared.readChunkSizeLimit = AppConfig::DEFAULT_READ_CHUNK_SIZE_LIMIT;
    m_shared.bufferSize         = AppConfig::DEFAULT_BUFFER_SIZE;
    m_shared.transfers          = AppConfig::DEFAULT_TRANSFERS;
    m_shared.checkers           = AppConfig::DEFAULT_CHECKERS;
}

bool SettingsManager::load()
{
    QFile file(m_filePath);
    if (!file.exists()) {
        // First run: write defaults and return false to signal the caller
        save();
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();

    if (root.contains("shared") && root["shared"].isObject()) {
        sharedFromJson(root["shared"].toObject());
    }

    m_services.clear();
    if (root.contains("services") && root["services"].isArray()) {
        for (const QJsonValue& v : root["services"].toArray()) {
            if (v.isObject()) {
                m_services.append(specFromJson(v.toObject()));
            }
        }
    }

    return true;
}

bool SettingsManager::save() const
{
    QJsonObject root;
    root["shared"] = sharedToJson();

    QJsonArray arr;
    for (const ServiceSpec& s : m_services) {
        arr.append(specToJson(s));
    }
    root["services"] = arr;

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void SettingsManager::addService(const ServiceSpec& spec)
{
    ServiceSpec s = spec;
    if (s.id.isEmpty()) {
        s.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    m_services.append(s);
}

void SettingsManager::updateService(const ServiceSpec& spec)
{
    for (ServiceSpec& s : m_services) {
        if (s.id == spec.id) {
            s = spec;
            return;
        }
    }
}

void SettingsManager::removeService(const QString& id)
{
    m_services.removeIf([&](const ServiceSpec& s) { return s.id == id; });
}

ServiceSpec* SettingsManager::findService(const QString& id)
{
    for (ServiceSpec& s : m_services) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// JSON helpers
// ---------------------------------------------------------------------------

QJsonObject SettingsManager::sharedToJson() const
{
    QJsonObject o;
    o["rclonePath"]         = m_shared.rclonePath;
    o["cacheMode"]          = m_shared.cacheMode;
    o["cacheMaxSize"]       = m_shared.cacheMaxSize;
    o["cacheMinFreeSpace"]  = m_shared.cacheMinFreeSpace;
    o["cacheMaxAge"]        = m_shared.cacheMaxAge;
    o["readChunkSize"]      = m_shared.readChunkSize;
    o["readChunkSizeLimit"] = m_shared.readChunkSizeLimit;
    o["bufferSize"]         = m_shared.bufferSize;
    o["transfers"]          = m_shared.transfers;
    o["checkers"]           = m_shared.checkers;
    return o;
}

void SettingsManager::sharedFromJson(const QJsonObject& o)
{
    auto str = [&](const char* k, const QString& def) -> QString {
        return o.contains(k) ? o[k].toString(def) : def;
    };
    auto num = [&](const char* k, int def) -> int {
        return o.contains(k) ? o[k].toInt(def) : def;
    };

    m_shared.rclonePath         = str("rclonePath",         AppConfig::DEFAULT_RCLONE_PATH);
    m_shared.cacheMode          = str("cacheMode",          AppConfig::DEFAULT_CACHE_MODE);
    m_shared.cacheMaxSize       = num("cacheMaxSize",       AppConfig::DEFAULT_CACHE_MAX_SIZE);
    m_shared.cacheMinFreeSpace  = num("cacheMinFreeSpace",  AppConfig::DEFAULT_CACHE_MIN_FREE_SPACE);
    m_shared.cacheMaxAge        = num("cacheMaxAge",        AppConfig::DEFAULT_CACHE_MAX_AGE);
    m_shared.readChunkSize      = num("readChunkSize",      AppConfig::DEFAULT_READ_CHUNK_SIZE);
    m_shared.readChunkSizeLimit = num("readChunkSizeLimit", AppConfig::DEFAULT_READ_CHUNK_SIZE_LIMIT);
    m_shared.bufferSize         = num("bufferSize",         AppConfig::DEFAULT_BUFFER_SIZE);
    m_shared.transfers          = num("transfers",          AppConfig::DEFAULT_TRANSFERS);
    m_shared.checkers           = num("checkers",           AppConfig::DEFAULT_CHECKERS);
}

QJsonObject SettingsManager::specToJson(const ServiceSpec& s) const
{
    QJsonObject o;
    o["id"]     = s.id;
    o["name"]   = s.name;
    o["local"]  = s.local;
    o["remote"] = s.remote;
    o["automount"] = s.automount;
    o["readOnly"] = s.readOnly;
    o["links"]  = s.links;
    return o;
}

ServiceSpec SettingsManager::specFromJson(const QJsonObject& o) const
{
    ServiceSpec s;
    s.id     = o["id"].toString();
    s.name   = o["name"].toString();
    s.local  = o["local"].toString();
    s.remote = o["remote"].toString();
    s.automount = o["automount"].toBool(false);
    s.readOnly = o["readOnly"].toBool(false);
    s.links  = o["links"].toBool(true);
    if (s.id.isEmpty()) {
        s.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    return s;
}
