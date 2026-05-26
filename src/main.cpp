#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SRCMG");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SRCMG");

    MainWindow w;
    w.show();

    return app.exec();
}
