// SPDX-License-Identifier: GPL-3.0-or-later
#include <sailfishapp.h>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDir>
#include <QLockFile>
#include <QStandardPaths>
#include <QTranslator>
#include "framare.h"

class Activation : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", FRAMARE_CONTROL_INTERFACE)
public:
    explicit Activation(QQuickView *view) : m_view(view) {}
public slots:
    void Activate() { m_view->show(); m_view->requestActivate(); }
private:
    QQuickView *m_view;
};

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setOrganizationName(QStringLiteral(FRAMARE_ORGANIZATION));
    app->setApplicationName(QStringLiteral(FRAMARE_INSTANCE));
    app->setApplicationDisplayName(QStringLiteral(FRAMARE_NAME));
    const QString data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!QDir().mkpath(data)) return 1;
    QFile::setPermissions(data, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QLockFile lock(data + "/session.lock");
    if (!lock.tryLock(0)) {
        QDBusInterface existing(QStringLiteral(FRAMARE_DBUS_NAME), "/Framare",
                                QStringLiteral(FRAMARE_CONTROL_INTERFACE));
        existing.call("Activate");
        return 0;
    }
    Framare backend(data, cache, QStringLiteral(FRAMARE_TRANSPORT));
    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty("framare", &backend);
    view->rootContext()->setContextProperty("applicationName", QStringLiteral(FRAMARE_NAME));
    view->setSource(SailfishApp::pathToMainQml());
    if (view->status() == QQuickView::Error) return 1;
    Activation activation(view.data());
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerObject("/Framare", &activation, QDBusConnection::ExportAllSlots)
            || !bus.registerService(QStringLiteral(FRAMARE_DBUS_NAME))) return 1;
    view->show();
    return app->exec();
}

#include "main.moc"
