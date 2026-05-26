#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SRCMG");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SRCMG");

    // Keep the app alive when the window is closed
    app.setQuitOnLastWindowClosed(false);

    MainWindow w;

    if (!QCoreApplication::arguments().contains("--tray"))
        w.show();

    return app.exec();
}
