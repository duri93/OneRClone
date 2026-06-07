#include "model/Config.h"
#include "view/MainWindow.h"
#include <QApplication>

#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(Config::APP_NAME);
    app.setApplicationVersion(Config::APP_VERSION);
    app.setOrganizationName(Config::APP_NAME);
    app.setQuitOnLastWindowClosed(false);

    // Try to connect to an already-running instance
    QLocalSocket socket;
    socket.connectToServer(Config::APP_ID);
    if (socket.waitForConnected(500)) {
        // Another instance is running — tell it to show itself and exit
        socket.write("show");
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        return 0;
    }

    // No existing instance — become the server
    QLocalServer server;
    QLocalServer::removeServer(Config::APP_ID); // clean up stale socket if crashed
    server.listen(Config::APP_ID);

    MainWindow window;

    // When a second instance connects, show the window
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket* conn = server.nextPendingConnection();
        auto fired = std::make_shared<bool>(false);

        QObject::connect(conn, &QLocalSocket::readyRead, [&window, conn, &fired]() {
            if (!*fired) {
                *fired = true;
                conn->readAll();
                window.activate();
            }
            conn->deleteLater();
        });
        QTimer::singleShot(200, conn, [&window, conn, fired]() {
            if (conn->bytesAvailable()) conn->readAll();
            if (!*fired) {
                *fired = true;
                window.activate();
            }
            conn->deleteLater();
        });
    });

    // show window
    if (!QCoreApplication::arguments().contains("--tray")){
        window.show();
    }

    return app.exec();
}





