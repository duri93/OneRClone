#pragma once

#include <QString>
#include <QJsonObject>

class SharedSettings
{
public:
    const QString rclonePath()         const { return m_rclonePath;         };
    const bool    advanced()           const { return m_advanced;           };
    const QString cacheMode()          const { return m_cacheMode;          };
    const int     cacheMaxSize()       const { return m_cacheMaxSize;       };
    const int     cacheMinFreeSpace()  const { return m_cacheMinFreeSpace;  };
    const int     cacheMaxAge()        const { return m_cacheMaxAge;        };
    const int     readChunkSize()      const { return m_readChunkSize;      };
    const int     readChunkSizeLimit() const { return m_readChunkSizeLimit; };
    const int     bufferSize()         const { return m_bufferSize;         };
    const int     transfers()          const { return m_transfers;          };
    const int     checkers()           const { return m_checkers;           };
    const bool    links()              const { return m_links;              };

    void setRclonePath        (QString newRclonePath        ){ m_rclonePath         = std::move(newRclonePath); };
    void setAdvanced          (bool    newAdvanced          ){ m_advanced           = newAdvanced;              };
    void setCacheMode         (QString newCacheMode         ){ m_cacheMode          = std::move(newCacheMode);  };
    void setCacheMaxSize      (int     newCacheMaxSize      ){ m_cacheMaxSize       = newCacheMaxSize;          };
    void setCacheMinFreeSpace (int     newCacheMinFreeSpace ){ m_cacheMinFreeSpace  = newCacheMinFreeSpace;     };
    void setCacheMaxAge       (int     newCacheMaxAge       ){ m_cacheMaxAge        = newCacheMaxAge;           };
    void setReadChunkSize     (int     newReadChunkSize     ){ m_readChunkSize      = newReadChunkSize;         };
    void setReadChunkSizeLimit(int     newReadChunkSizeLimit){ m_readChunkSizeLimit = newReadChunkSizeLimit;    };
    void setBufferSize        (int     newBufferSize        ){ m_bufferSize         = newBufferSize;            };
    void setTransfers         (int     newTransfers         ){ m_transfers          = newTransfers;             };
    void setCheckers          (int     newCheckers          ){ m_checkers           = newCheckers;              };
    void setLinks             (bool    newLinks             ){ m_links              = newLinks;                 };

    SharedSettings();
    SharedSettings(const QJsonObject& o);

    const QJsonObject toJson() const;
private:
    QString m_rclonePath         = "C:\\RClone\\rclone.exe";
    bool    m_advanced           = false;
    QString m_cacheMode          = "full";
    int     m_cacheMaxSize       = 50;    // GB
    int     m_cacheMinFreeSpace  = 50;    // GB
    int     m_cacheMaxAge        = 24;    // hours
    int     m_readChunkSize      = 128;   // MB
    int     m_readChunkSizeLimit = 2048;  // MB
    int     m_bufferSize         = 256;   // MB
    int     m_transfers          = 16;
    int     m_checkers           = 16;
    bool    m_links              = true;
};
