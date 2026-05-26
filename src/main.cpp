#include "mainwindow.h"
#include <QApplication>

#include <QRect>
#include <QSize>
#include <QScreen>

#include <QLocalServer>
#include <QLocalSocket>

static const char* INSTANCE_KEY = "SRCMG_SINGLE_INSTANCE";

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SRCMG");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SRCMG");
    app.setQuitOnLastWindowClosed(false);

    // Try to connect to an already-running instance
    QLocalSocket socket;
    socket.connectToServer(INSTANCE_KEY);
    if (socket.waitForConnected(500)) {
        // Another instance is running — tell it to show itself and exit
        socket.write("show");
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        return 0;
    }

    // No existing instance — become the server
    QLocalServer server;
    QLocalServer::removeServer(INSTANCE_KEY); // clean up stale socket if crashed
    server.listen(INSTANCE_KEY);

    MainWindow w;

    // When a second instance connects, show the window
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket* conn = server.nextPendingConnection();
        QObject::connect(conn, &QLocalSocket::readyRead, [&w, conn]() {
            conn->readAll(); // consume the message
            w.show();
            w.setWindowState( (w.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            w.raise();
            w.activateWindow();

            conn->deleteLater();
        });
    });

    // position window in bottom-right corner
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect available = screen->availableGeometry(); // excludes taskbar
    QSize  size     = w.frameSize().isEmpty() ? w.sizeHint() : w.frameSize();

    int x = available.right()  - size.width()  - 6;
    int y = available.bottom() - size.height() - 38;
    w.move(x, y);

    // show window
    if (!QCoreApplication::arguments().contains("--tray")){
        w.show();
    }

    return app.exec();
}
