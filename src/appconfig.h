#pragma once
#include <QString>

namespace AppConfig {

// Application info
inline constexpr char APP_NAME[]    = "SRCMG";
inline constexpr char APP_VERSION[] = "1.0.0";

// Settings file (resolved at runtime relative to exe)
inline constexpr char SETTINGS_FILENAME[] = "settings.json";

// Error detection: regex applied against rclone stdout
// Replace this with whatever pattern signals a mount error
inline constexpr char DEFAULT_ERROR_REGEX[] = "NOTICE:.*failed|ERROR:";
inline constexpr char DEFAULT_START_STRING[] = "The service rclone has been started.";

// Default values for shared settings
inline constexpr char    DEFAULT_RCLONE_PATH[]         = "C:\\RClone\\rclone.exe";
inline constexpr char    DEFAULT_CACHE_MODE[]           = "full";
inline constexpr int     DEFAULT_CACHE_MAX_SIZE        = 50;    // GB
inline constexpr int     DEFAULT_CACHE_MIN_FREE_SPACE  = 50;    // GB
inline constexpr int     DEFAULT_CACHE_MAX_AGE         = 24;    // hours
inline constexpr int     DEFAULT_READ_CHUNK_SIZE       = 128;   // MB
inline constexpr int     DEFAULT_READ_CHUNK_SIZE_LIMIT = 2048;  // MB
inline constexpr int     DEFAULT_BUFFER_SIZE           = 256;   // MB
inline constexpr int     DEFAULT_TRANSFERS             = 16;
inline constexpr int     DEFAULT_CHECKERS              = 16;

} // namespace AppConfig
