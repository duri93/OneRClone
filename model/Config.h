#pragma once

#include <QString>
#include <QRegularExpression>

namespace Config {

    // Application info
    inline constexpr char APP_NAME[]    = "OneRClone";
    inline constexpr char APP_VERSION[] = "2.1";
    inline constexpr char APP_ID[]      = "tk.duri.onerclone";

    // Settings file (resolved at runtime relative to exe)
    inline constexpr char SETTINGS_FILENAME[] = "settings.json";

    // Status detection: regex applied against rclone stdout
    inline constexpr char DEFAULT_ERROR_REGEX[] = "NOTICE:.*failed|ERROR:";
    inline constexpr char DEFAULT_START_STRING[] = "The service rclone has been started.";

    // display
    inline constexpr int STATUS_DURATION = 5000;
    inline constexpr int MAX_OUTPUT_LINES = 2000;

    inline const QRegularExpression ERROR_REGEX{
        Config::DEFAULT_ERROR_REGEX
    };

}