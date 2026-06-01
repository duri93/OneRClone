#include "SharedSettings.h"

SharedSettings::SharedSettings() {}

SharedSettings::SharedSettings(const QJsonObject& o){
    auto str = [&](const char* k, const QString& def) -> QString {
        return o.contains(k) ? o[k].toString(def) : def;
    };
    auto num = [&](const char* k, int def) -> int {
        return o.contains(k) ? o[k].toInt(def) : def;
    };
    auto log = [&](const char* k, bool def) -> bool {
        return o.contains(k) ? o[k].toBool(def) : def;
    };

    this->m_rclonePath         = str("rclonePath",         this->rclonePath());
    this->m_advanced           = log("advanced",           this->advanced());
    this->m_cacheMode          = str("cacheMode",          this->cacheMode());
    this->m_cacheMaxSize       = num("cacheMaxSize",       this->cacheMaxSize());
    this->m_cacheMinFreeSpace  = num("cacheMinFreeSpace",  this->cacheMinFreeSpace());
    this->m_cacheMaxAge        = num("cacheMaxAge",        this->cacheMaxAge());
    this->m_readChunkSize      = num("readChunkSize",      this->readChunkSize());
    this->m_readChunkSizeLimit = num("readChunkSizeLimit", this->readChunkSizeLimit());
    this->m_bufferSize         = num("bufferSize",         this->bufferSize());
    this->m_transfers          = num("transfers",          this->transfers());
    this->m_checkers           = num("checkers",           this->checkers());
    this->m_links              = log("links",              this->links());
}

const QJsonObject SharedSettings::toJson() const{
    QJsonObject o;

    o["rclonePath"]         = this->rclonePath();
    o["advanced"]           = this->advanced();
    o["cacheMode"]          = this->cacheMode();
    o["cacheMaxSize"]       = this->cacheMaxSize();
    o["cacheMinFreeSpace"]  = this->cacheMinFreeSpace();
    o["cacheMaxAge"]        = this->cacheMaxAge();
    o["readChunkSize"]      = this->readChunkSize();
    o["readChunkSizeLimit"] = this->readChunkSizeLimit();
    o["bufferSize"]         = this->bufferSize();
    o["transfers"]          = this->transfers();
    o["checkers"]           = this->checkers();
    o["links"]              = this->links();

    return o;
}