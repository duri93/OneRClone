#pragma once

#include <QString>
#include <QRegularExpression>

namespace Config {

    // Application info
    inline constexpr char APP_NAME[]    = "OneClone";
    inline constexpr char APP_VERSION[] = "1.3";
    inline constexpr char APP_ID[]      = "tk.duri.oneclone";

    // Settings file (resolved at runtime relative to exe)
    inline constexpr char SETTINGS_FILENAME[] = "settings.json";

    // Status detection: regex applied against rclone stdout
    inline constexpr char DEFAULT_START_STRING[] = "The service rclone has been started.";

    // display
    inline constexpr int STATUS_DURATION = 5000;
    inline constexpr int MAX_OUTPUT_LINES = 2000;
    inline constexpr int SMALL_FONT_SIZE = 8;

    inline const QRegularExpression WARNING_REGEX{
        "NOTICE:.*failed|ERROR:"
    };

}