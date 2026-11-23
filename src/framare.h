// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QVariantList>
#include <QJsonObject>
#include <QProcess>
#include <QTimer>
#include <QTemporaryDir>
#include <QFutureWatcher>
#include <QSettings>
#include <memory>

struct PreparedImage {
    QString path;
    QString error;
};

class TransportProcess : public QProcess {
public:
    TransportProcess();
protected:
    void setupChildProcess() override;
private:
    qint64 m_parentPid;
};

class Framare : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList frames READ frames NOTIFY framesChanged)
    Q_PROPERTY(QVariantList jobs READ jobs NOTIFY jobsChanged)
    Q_PROPERTY(QString defaultFrame READ defaultFrame NOTIFY framesChanged)
    Q_PROPERTY(QString senderName READ senderName NOTIFY framesChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
public:
    explicit Framare(const QString &dataDir, const QString &cacheDir,
                     const QString &program, QObject *parent = nullptr, int timeoutMs = 120000);
    ~Framare() override;
    QVariantList frames() const;
    QVariantList jobs() const;
    QString defaultFrame() const;
    QString senderName() const;
    QString error() const { return m_error; }
    bool busy() const { return !m_operation.isEmpty() || m_current >= 0 || m_preparing >= 0; }

    Q_INVOKABLE void pair(const QString &name, const QString &code);
    Q_INVOKABLE void forget(const QString &name);
    Q_INVOKABLE void setDefaultFrame(const QString &name);
    Q_INVOKABLE void setSenderName(const QString &name);
    Q_INVOKABLE void addImages(const QStringList &paths);
    Q_INVOKABLE void send(const QString &frame, const QString &caption, bool fit);
    Q_INVOKABLE void retry(int index, const QString &frame);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void dismissError() { setError(QString()); }
    Q_INVOKABLE void unsupportedResource();

signals:
    void framesChanged();
    void jobsChanged();
    void errorChanged();
    void busyChanged();

private:
    struct Job {
        QString source, cached, name, state, message, frame, caption;
        bool fit = false;
    };
    void setError(const QString &message);
    bool loadConfig();
    bool saveConfig();
    void prepareNext();
    void runNext();
    void startProcess(const QString &operation, const QStringList &arguments);
    void finishProcess(bool success);
    void interrupt();
    void drainOutput();
    QString m_configPath, m_program, m_error, m_operation, m_pairName;
    QJsonObject m_config;
    bool m_configValid = true, m_cancelled = false, m_timedOut = false;
    int m_current = -1, m_preparing = -1;
    int m_timeoutMs;
    QList<Job> m_jobs;
    TransportProcess m_process;
    QTimer m_deadline, m_kill;
    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<QTemporaryDir> m_cache;
    QFutureWatcher<PreparedImage> m_prepare;
};
