// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QTemporaryDir>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QBuffer>
#include "framare.h"

namespace {
QByteArray bytes(const QString &path) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly)) return QByteArray(); return f.readAll();
}
void write(const QString &path, const QByteArray &data) {
    QFile f(path); if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size()) qFatal("fixture write failed");
}
QString image(const QString &root, const QString &name) {
    const QString path = root + "/" + name;
    QImage picture(24, 16, QImage::Format_RGB32); picture.fill(Qt::blue);
    if (!picture.save(path, "PNG")) qFatal("fixture image failed");
    return path;
}
QString transport(const QString &root) {
    const QString path = root + "/fake transport.py";
    if (!QFile::copy(QStringLiteral(FIXTURE_DIR) + "/fake_transport.py", path)) qFatal("fixture copy failed");
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    return path;
}
void config(const QString &root) {
    QDir().mkpath(root + "/data");
    QJsonObject frames;
    frames.insert("Frame A", QJsonObject{{"peer_id", QString(64, '2')}});
    frames.insert(QString::fromUtf8("Küche B"), QJsonObject{{"peer_id", QString(64, '3')}});
    QJsonObject object{{"private_key", QString(64, '1')}, {"frames", frames}, {"default_frame", "Frame A"}};
    write(root + "/data/identity.json", QJsonDocument(object).toJson());
}
QString state(const Framare &app, int index) { return app.jobs().at(index).toMap().value("state").toString(); }
QList<QJsonArray> calls(const QString &root) {
    QList<QJsonArray> result;
    for (const QByteArray &line : bytes(root + "/calls.jsonl").split('\n'))
        if (!line.isEmpty()) result.append(QJsonDocument::fromJson(line).array());
    return result;
}
}

class Tests : public QObject {
    Q_OBJECT
private slots:
    void corruptIdentityIsPreserved() {
        QTemporaryDir root; QVERIFY(root.isValid());
        QDir().mkpath(root.path() + "/data");
        const QByteArray original = "broken identity - do not overwrite";
        const QString path = root.path() + "/data/identity.json";
        write(path, original);
        Framare app(root.path()+"/data", root.path()+"/cache", transport(root.path()));
        QVERIFY(!app.error().isEmpty());
        app.pair("New frame", "1234567890");
        QCOMPARE(bytes(path), original);
        QVERIFY(calls(root.path()).isEmpty());
        QVERIFY(!app.busy());
    }

    void frameSettingsPreserveIdentity() {
        QTemporaryDir root; config(root.path());
        const QString file = root.path()+"/data/identity.json";
        {
            Framare app(root.path()+"/data", root.path()+"/cache", transport(root.path()));
            QCOMPARE(app.frames().size(), 2);
            app.setDefaultFrame(QString::fromUtf8("Küche B"));
            app.setSenderName(QString::fromUtf8("Möwe ; $(not-a-command)"));
            app.forget("Frame A");
            QCOMPARE(app.frames().size(), 1);
            QCOMPARE(app.defaultFrame(), QString::fromUtf8("Küche B"));
        }
        const QJsonObject saved = QJsonDocument::fromJson(bytes(file)).object();
        QCOMPARE(saved.value("private_key").toString(), QString(64, '1'));
        QVERIFY(!(QFile::permissions(file) & (QFile::ReadGroup | QFile::ReadOther | QFile::WriteGroup | QFile::WriteOther)));
        Framare restarted(root.path()+"/data", root.path()+"/cache", "/unused");
        QCOMPARE(restarted.defaultFrame(), QString::fromUtf8("Küche B"));
        QCOMPARE(restarted.senderName(), QString::fromUtf8("Möwe ; $(not-a-command)"));
        restarted.forget(QString::fromUtf8("Küche B"));
        QCOMPARE(restarted.frames().size(), 0);
        QCOMPARE(QJsonDocument::fromJson(bytes(file)).object().value("private_key").toString(), QString(64, '1'));
    }

    void missingPreviousIdentityIsNotRegenerated() {
        QTemporaryDir root; config(root.path());
        {
            Framare app(root.path()+"/data", root.path()+"/cache", "/unused");
            QCOMPARE(app.frames().size(), 2);
        }
        QVERIFY(QFile::remove(root.path()+"/data/identity.json"));
        Framare app(root.path()+"/data", root.path()+"/cache", transport(root.path()));
        app.pair("New frame", "1234567890");
        QVERIFY(!app.error().isEmpty());
        QVERIFY(!QFile::exists(root.path()+"/data/identity.json"));
        QVERIFY(calls(root.path()).isEmpty());
    }

    void resourcesAreValidatedAndOriginalsPreserved() {
        QTemporaryDir root; config(root.path());
        const QString original = image(root.path(), QString::fromUtf8("Möwe ; $quote.txt"));
        const QByteArray before = bytes(original);
        const QString fake = root.path()+"/pretend.jpg";
        write(fake, "not a picture");
        Framare app(root.path()+"/data", root.path()+"/cache", "/unused");
        app.addImages({original, QUrl::fromLocalFile(original).toString(), "https://example.invalid/image.jpg", fake});
        QTRY_VERIFY(!app.busy());
        QCOMPARE(app.jobs().size(), 2);
        QVERIFY2(state(app,0) == "Ready", qPrintable(app.jobs()[0].toMap()["message"].toString()));
        QCOMPARE(state(app,1), QString("Failed"));
        const QString copy = app.jobs()[0].toMap()["preview"].toUrl().toLocalFile();
        QVERIFY(QFile::exists(copy));
        QVERIFY(copy != original);
        app.clear();
        QVERIFY(!QFile::exists(copy));
        QCOMPARE(bytes(original), before);
    }

    void queueUsesSelectedFrameAndLiteralArguments() {
        QTemporaryDir root; config(root.path());
        const QString fake = transport(root.path());
        write(root.path()+"/fail-second", "");
        const QString first = image(root.path(), "one.png");
        const QString second = image(root.path(), "two.png");
        Framare app(root.path()+"/data", root.path()+"/cache", fake);
        app.addImages({first, second});
        QTRY_VERIFY(!app.busy());
        const QString caption = QString::fromUtf8("Grüße ; $(touch HACKED) \" ' &");
        app.send(QString::fromUtf8("Küche B"), caption, true);
        QTRY_VERIFY(!app.busy());
        QCOMPARE(state(app,0), QString("Sent"));
        QCOMPARE(state(app,1), QString("Failed"));
        QCOMPARE(calls(root.path()).size(), 2);
        for (const QJsonArray &arguments : calls(root.path())) {
            QStringList list;
            for (const QJsonValue &value : arguments) list << value.toString();
            QCOMPARE(list.value(list.indexOf("-frame")+1), QString::fromUtf8("Küche B"));
            QCOMPARE(list.value(list.indexOf("-caption")+1), caption);
            QVERIFY(list.contains("-fit"));
            QCOMPARE(list.value(list.indexOf("-config")+1), root.path()+"/data/identity.json");
        }
        app.forget(QString::fromUtf8("Küche B"));
        app.retry(0, "Frame A"); // Acknowledged images must never be retried by this action.
        app.retry(1, "Frame A");
        QTRY_VERIFY(!app.busy());
        QCOMPARE(calls(root.path()).size(), 3);
        QCOMPARE(state(app,0), QString("Sent"));
        QCOMPARE(state(app,1), QString("Sent"));
        QCOMPARE(app.jobs()[1].toMap()["frame"].toString(), QString("Frame A"));
        QVERIFY(!QFile::exists("HACKED"));
    }

    void exifOrientationIsAppliedToPrivateCopy() {
        QTemporaryDir root; config(root.path());
        QImage picture(24,16,QImage::Format_RGB32); picture.fill(Qt::green);
        QByteArray jpeg;
        QBuffer buffer(&jpeg); QVERIFY(buffer.open(QIODevice::WriteOnly));
        QVERIFY(picture.save(&buffer,"JPEG"));
        const QByteArray exif = QByteArray::fromHex("45786966000049492a0008000000010012010300010000000600000000000000");
        jpeg.insert(2, QByteArray::fromHex("ffe10022") + exif);
        const QString original=root.path()+"/rotated.jpg";
        write(original,jpeg);
        Framare app(root.path()+"/data",root.path()+"/cache","/unused");
        app.addImages({original});
        QTRY_VERIFY(!app.busy());
        QVERIFY2(state(app,0)=="Ready",qPrintable(app.jobs()[0].toMap()["message"].toString()));
        const QImage prepared(app.jobs()[0].toMap()["preview"].toUrl().toLocalFile());
        QCOMPARE(prepared.size(),QSize(16,24));
        QCOMPARE(bytes(original),jpeg);
    }

    void cancelStopsQueueAndKeepsIdentity() {
        QTemporaryDir root; config(root.path());
        const QByteArray identity = bytes(root.path()+"/data/identity.json");
        write(root.path()+"/hang", "");
        Framare app(root.path()+"/data", root.path()+"/cache", transport(root.path()));
        app.addImages({image(root.path(),"one.png"), image(root.path(),"two.png")});
        QTRY_VERIFY(!app.busy());
        app.send("Frame A", "", false);
        QTRY_COMPARE(calls(root.path()).size(), 1);
        app.cancel();
        QTRY_VERIFY(!app.busy());
        QCOMPARE(state(app,0), QString("Cancelled"));
        QCOMPARE(state(app,1), QString("Cancelled"));
        QCOMPARE(calls(root.path()).size(), 1);
        QCOMPARE(bytes(root.path()+"/data/identity.json"), identity);
    }

    void watchdogAndFailedStartAreTerminal() {
        QTemporaryDir root; config(root.path());
        write(root.path()+"/hang", "");
        const QString picture = image(root.path(),"one.png");
        {
            Framare app(root.path()+"/data", root.path()+"/cache", transport(root.path()), nullptr, 150);
            app.addImages({picture});
            QTRY_VERIFY(!app.busy());
            app.send("Frame A", "", false);
            QTRY_VERIFY(!app.busy());
            QCOMPARE(state(app,0), QString("Failed"));
            QVERIFY(app.jobs()[0].toMap()["message"].toString().contains("timed out"));
        }
        Framare app(root.path()+"/data", root.path()+"/cache", root.path()+"/missing-program");
        app.addImages({picture});
        QTRY_VERIFY(!app.busy());
        app.send("Frame A", "", false);
        QTRY_VERIFY(!app.busy());
        QCOMPARE(state(app,0), QString("Failed"));
    }

    void pairingNamesAreSeparateArguments() {
        QTemporaryDir root;
        Framare app(root.path()+"/data", root.path()+"/cache", transport(root.path()));
        const QString name = QString::fromUtf8("Wohnzimmer ; ' €");
        app.pair(name,"1234-567-890");
        QTRY_VERIFY(!app.busy());
        QCOMPARE(app.frames().size(), 1);
        QCOMPARE(app.frames()[0].toMap()["name"].toString(), name);
        QCOMPARE(app.senderName(), QString("Framare"));
        QCOMPARE(calls(root.path()).size(), 1);
    }

    void transportExitsWhenApplicationIsKilled() {
#ifdef Q_OS_LINUX
        QTemporaryDir root; config(root.path());
        transport(root.path());
        image(root.path(),"one.png");
        write(root.path()+"/hang", "");
        QProcess application;
        application.start(QCoreApplication::applicationFilePath(), {"--orphan-probe",root.path()});
        QVERIFY(application.waitForStarted());
        QTRY_VERIFY(bytes(root.path()+"/transport.pid").toLongLong()>0);
        const qint64 pid=bytes(root.path()+"/transport.pid").toLongLong();
        application.kill();
        QVERIFY(application.waitForFinished());
        const auto stillRunning=[pid]() {
            const QByteArray status=bytes(QString("/proc/%1/stat").arg(pid));
            if (status.isEmpty()) return false;
            const QByteArray state=status.mid(status.lastIndexOf(')')+2,1);
            return state!="Z" && state!="X";
        };
        QTRY_VERIFY(!stillRunning());
#endif
    }
};

int main(int argc,char **argv)
{
    QCoreApplication application(argc,argv);
    if (application.arguments().value(1)=="--orphan-probe") {
        const QString root=application.arguments().value(2);
        Framare backend(root+"/data",root+"/cache",root+"/fake transport.py");
        QTimer start;
        QObject::connect(&start,&QTimer::timeout,[&]() {
            if (!backend.busy() && !backend.jobs().isEmpty() && state(backend,0)=="Ready") {
                start.stop(); backend.send("Frame A","",false);
            }
        });
        backend.addImages({root+"/one.png"});
        start.start(5);
        return application.exec();
    }
    Tests tests;
    return QTest::qExec(&tests,argc,argv);
}
#include "tst_framare.moc"
