// SPDX-License-Identifier: GPL-3.0-or-later
#include "framare.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QImageReader>
#include <QImage>
#include <QSaveFile>
#include <QRegularExpression>
#include <QUrl>
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>
#include <signal.h>
#include <unistd.h>
#ifdef Q_OS_LINUX
#include <sys/prctl.h>
#endif

TransportProcess::TransportProcess() : m_parentPid(::getpid()) {}

void TransportProcess::setupChildProcess()
{
#ifdef Q_OS_LINUX
    // Only async-signal-safe operations between fork and exec. The second
    // check closes the race where the parent died before prctl was installed.
    if (::prctl(PR_SET_PDEATHSIG, SIGTERM, 0UL, 0UL, 0UL) != 0 || ::getppid() != m_parentPid)
        ::_exit(125);
#endif
}

namespace {
const QFile::Permissions ownerOnly = QFile::ReadOwner | QFile::WriteOwner;
PreparedImage prepareImage(const QString &source, const QString &directory)
{
    const QFileInfo info(source);
    if (!info.isFile() || !info.isReadable() || info.size() <= 0 || info.size() > 32 * 1024 * 1024)
        return {QString(), Framare::tr("The image is unreadable or larger than 32 MB.")};
    QImageReader reader(source);
    reader.setDecideFormatFromContent(true);
    reader.setAutoTransform(true);
    const QByteArray format = reader.format().toLower();
    const QSize size = reader.size();
    if ((format != "jpeg" && format != "jpg" && format != "png") || !size.isValid())
        return {QString(), Framare::tr("Only valid JPEG and PNG images are supported.")};
    if (qint64(size.width()) * size.height() > 40000000)
        return {QString(), Framare::tr("The image is larger than 40 megapixels.")};
    const QImage image = reader.read();
    if (image.isNull()) return {QString(), Framare::tr("The image could not be decoded.")};
    const QString output = directory + "/" + QUuid::createUuid().toString() + ".jpg";
    if (!image.save(output, "JPEG", 95) || !QFile::setPermissions(output, ownerOnly)) {
        QFile::remove(output);
        return {QString(), Framare::tr("The private image copy could not be saved.")};
    }
    return {output, QString()};
}
}

Framare::Framare(const QString &dataDir, const QString &cacheDir,
                 const QString &program, QObject *parent, int timeoutMs)
    : QObject(parent), m_configPath(dataDir + "/identity.json"), m_program(program), m_timeoutMs(timeoutMs)
{
    QDir().mkpath(dataDir);
    QDir().mkpath(cacheDir);
    const QFile::Permissions directoryMode = ownerOnly | QFile::ExeOwner;
    const bool privateDirs = QFile::setPermissions(dataDir, directoryMode)
            && QFile::setPermissions(cacheDir, directoryMode);
    m_settings.reset(new QSettings(dataDir + "/settings.ini", QSettings::IniFormat));
    // Only our own transfer-* directories are eligible for stale-cache cleanup.
    const QDir cache(cacheDir);
    for (const QString &name : cache.entryList({"transfer-*"}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QFileInfo entry(cache.filePath(name));
        if (!entry.isSymLink()) QDir(entry.absoluteFilePath()).removeRecursively();
    }
    m_cache.reset(new QTemporaryDir(cacheDir + "/transfer-XXXXXX"));
    if (!privateDirs || !m_cache->isValid()) {
        m_configValid = false;
        setError(tr("Private application storage is not writable."));
    } else loadConfig();

    m_deadline.setSingleShot(true);
    m_kill.setSingleShot(true);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &Framare::drainOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, &Framare::drainOutput);
    connect(&m_process, static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
        finishProcess(code == 0 && status == QProcess::NormalExit);
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) finishProcess(false);
    });
    connect(&m_process, &QProcess::started, this, [this] {
        if (m_cancelled || m_timedOut) interrupt();
    });
    connect(&m_deadline, &QTimer::timeout, this, [this] { m_timedOut = true; interrupt(); });
    connect(&m_kill, &QTimer::timeout, this, [this] {
        if (m_process.state() != QProcess::NotRunning) m_process.kill();
    });
    connect(&m_prepare, &QFutureWatcher<PreparedImage>::finished, this, [this] {
        const PreparedImage result = m_prepare.result();
        Job &job = m_jobs[m_preparing];
        qint64 bytes = QFileInfo(result.path).size();
        for (const Job &existing : m_jobs) bytes += QFileInfo(existing.cached).size();
        if (job.state == "Cancelled") {
            QFile::remove(result.path);
        } else if (!result.error.isEmpty()) {
            job.state = "Failed";
            job.message = result.error;
        } else if (bytes > 128 * 1024 * 1024) {
            QFile::remove(result.path);
            job.state = "Failed";
            job.message = tr("The private image cache is full. Clear completed images first.");
        } else {
            job.cached = result.path;
            job.state = "Ready";
        }
        m_preparing = -1;
        emit jobsChanged();
        emit busyChanged();
        prepareNext();
    });
}

Framare::~Framare()
{
    m_deadline.stop(); m_kill.stop();
    disconnect(&m_process, nullptr, this, nullptr);
    m_prepare.waitForFinished();
    if (m_process.state() != QProcess::NotRunning) {
        if (m_process.processId() > 0) ::kill(m_process.processId(), SIGINT);
        if (!m_process.waitForFinished(1500)) { m_process.kill(); m_process.waitForFinished(1500); }
    }
}

void Framare::setError(const QString &message) { m_error = message; emit errorChanged(); }

bool Framare::loadConfig()
{
    QFile file(m_configPath);
    if (!file.exists()) {
        if (m_settings->value("identityCreated", false).toBool()) {
            m_configValid = false;
            setError(tr("The pairing identity is missing. Restore its backup before continuing."));
            return false;
        }
        m_config = QJsonObject();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024) {
        m_configValid = false;
    } else {
        QJsonParseError parse;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse);
        const QJsonObject object = document.object();
        m_configValid = parse.error == QJsonParseError::NoError && document.isObject()
                && QRegularExpression("^[a-fA-F0-9]{64}$").match(object.value("private_key").toString()).hasMatch()
                && (!object.contains("frames") || object.value("frames").isObject());
        if (m_configValid) m_config = object;
    }
    if (!m_configValid) {
        setError(tr("The pairing data is damaged or unreadable. It has not been replaced."));
        return false;
    }
    if (!QFile::setPermissions(m_configPath, ownerOnly)) {
        m_configValid = false;
        setError(tr("The pairing identity could not be protected."));
        return false;
    }
    m_settings->setValue("identityCreated", true);
    m_settings->sync();
    QFile::setPermissions(m_settings->fileName(), ownerOnly);
    emit framesChanged();
    return true;
}

bool Framare::saveConfig()
{
    if (!m_configValid || m_config.isEmpty()) return false;
    QSaveFile file(m_configPath);
    const QByteArray bytes = QJsonDocument(m_config).toJson();
    if (!file.open(QIODevice::WriteOnly) || !file.setPermissions(ownerOnly)
            || file.write(bytes) != bytes.size() || !file.commit()) {
        setError(tr("The pairing data could not be saved."));
        loadConfig();
        return false;
    }
    emit framesChanged();
    return true;
}

QVariantList Framare::frames() const
{
    QVariantList result;
    if (!m_configValid) return result;
    const QJsonObject frames = m_config.value("frames").toObject();
    for (auto it = frames.begin(); it != frames.end(); ++it)
        result.append(QVariantMap{{"name", it.key()}, {"isDefault", it.key() == defaultFrame()}});
    return result;
}

QString Framare::defaultFrame() const { return m_config.value("default_frame").toString(); }
QString Framare::senderName() const {
    return m_settings->value("senderName", m_config.value("client_name").toString("Framare")).toString();
}

QVariantList Framare::jobs() const
{
    QVariantList result;
    for (const Job &job : m_jobs)
        result.append(QVariantMap{{"name", job.name}, {"state", job.state},
                                  {"message", job.message}, {"frame", job.frame},
                                  {"preview", job.cached.isEmpty() ? QUrl() : QUrl::fromLocalFile(job.cached)}});
    return result;
}

void Framare::pair(const QString &name, const QString &code)
{
    if (busy() || !loadConfig()) return;
    QString digits;
    for (QChar c : code) if (c >= QLatin1Char('0') && c <= QLatin1Char('9')) digits += c;
    const QString label = name.trimmed();
    if (label.isEmpty() || label.size() > 80 || digits.size() < 7 || digits.size() > 31) {
        setError(tr("Enter a frame name and a valid friendship code.")); return;
    }
    if (m_config.value("frames").toObject().contains(label)) {
        setError(tr("A frame with this name already exists.")); return;
    }
    m_pairName = label;
    m_settings->setValue("senderName", senderName());
    m_settings->sync();
    startProcess("pair", {"-frame", label, "pair", digits});
}

void Framare::forget(const QString &name)
{
    if (busy() || !m_configValid) return;
    QJsonObject frames = m_config.value("frames").toObject();
    if (!frames.contains(name)) return;
    frames.remove(name);
    m_config.insert("frames", frames);
    if (defaultFrame() == name) m_config.insert("default_frame", frames.isEmpty() ? QString() : frames.keys().first());
    saveConfig();
}

void Framare::setDefaultFrame(const QString &name)
{
    if (busy() || !m_configValid || !m_config.value("frames").toObject().contains(name)) return;
    m_config.insert("default_frame", name); saveConfig();
}

void Framare::setSenderName(const QString &name)
{
    if (busy() || !m_configValid || name.trimmed().isEmpty() || name.size() > 80) return;
    m_settings->setValue("senderName", name.trimmed());
    m_settings->sync();
    if (!m_config.isEmpty()) { m_config.insert("client_name", name.trimmed()); saveConfig(); }
    emit framesChanged();
}

void Framare::unsupportedResource() { setError(tr("Only local image files can be shared with Framare.")); }

void Framare::addImages(const QStringList &paths)
{
    for (const QString &input : paths) {
        if (m_jobs.size() >= 16) { setError(tr("A queue can contain at most 16 images.")); break; }
        QString path = input;
        if (!QDir::isAbsolutePath(path)) {
            const QUrl url(input);
            if (!url.isLocalFile() || (!url.host().isEmpty() && url.host() != "localhost")) {
                unsupportedResource(); continue;
            }
            path = url.toLocalFile();
        }
        const QFileInfo info(path);
        path = info.canonicalFilePath();
        if (path.isEmpty() || !info.isFile() || !info.isReadable()) {
            setError(tr("The shared image is not accessible.")); continue;
        }
        bool duplicate = false;
        for (const Job &job : m_jobs) if (job.source == path) duplicate = true;
        if (duplicate) continue;
        Job job;
        job.source = path; job.name = info.fileName(); job.state = "Preparing";
        m_jobs.append(job);
    }
    emit jobsChanged();
    prepareNext();
}

void Framare::prepareNext()
{
    if (m_preparing >= 0 || !m_cache->isValid()) return;
    for (int i = 0; i < m_jobs.size(); ++i) if (m_jobs[i].state == "Preparing") {
        m_preparing = i;
        emit busyChanged();
        m_prepare.setFuture(QtConcurrent::run(prepareImage, m_jobs[i].source, m_cache->path()));
        return;
    }
}

void Framare::send(const QString &frame, const QString &caption, bool fit)
{
    if (busy() || !m_configValid) return;
    if (!m_config.value("frames").toObject().contains(frame)) {
        setError(tr("Select a paired frame first.")); return;
    }
    if (caption.size() > 1000) { setError(tr("The caption is too long.")); return; }
    for (Job &job : m_jobs) if (job.state == "Ready") {
        job.frame = frame; job.caption = caption; job.fit = fit; job.state = "Queued";
    }
    emit jobsChanged(); runNext();
}

void Framare::retry(int index, const QString &frame)
{
    if (busy() || !m_configValid || index < 0 || index >= m_jobs.size()) return;
    Job &job = m_jobs[index];
    if ((job.state != "Failed" && job.state != "Cancelled") || job.cached.isEmpty()) return;
    if (!m_config.value("frames").toObject().contains(frame)) {
        setError(tr("Select a paired frame first.")); return;
    }
    job.frame = frame;
    job.state = "Queued"; job.message.clear(); emit jobsChanged(); runNext();
}

void Framare::runNext()
{
    if (!m_operation.isEmpty()) return;
    for (int i = 0; i < m_jobs.size(); ++i) if (m_jobs[i].state == "Queued") {
        m_current = i;
        Job &job = m_jobs[i]; job.state = "Sending"; emit jobsChanged();
        QStringList args = {"-frame", job.frame, "-caption", job.caption};
        if (job.fit) args << "-fit";
        args << "send" << job.cached;
        startProcess("send", args);
        return;
    }
    emit busyChanged();
}

void Framare::drainOutput()
{
    // Output may contain private filenames, captions or pairing data. Discard it
    // in bounded chunks; success is determined solely by process termination.
    for (QProcess::ProcessChannel channel : {QProcess::StandardOutput, QProcess::StandardError}) {
        m_process.setReadChannel(channel);
        while (m_process.bytesAvailable() > 0) m_process.read(4096);
    }
}

void Framare::startProcess(const QString &operation, const QStringList &arguments)
{
    setError(QString());
    m_operation = operation; m_cancelled = false; m_timedOut = false;
    m_deadline.start(m_timeoutMs);
    emit busyChanged();
    m_process.setProgram(m_program);
    // The UI watchdog expires first, so its time limit has a reliable error
    // classification. The CLI deadline remains a fallback if the UI is paused.
    m_process.setArguments(QStringList{"-config", m_configPath, "-timeout", "3m"} + arguments);
    m_process.start();
}

void Framare::interrupt()
{
    if (m_process.state() != QProcess::NotRunning && m_process.processId() > 0) {
        ::kill(m_process.processId(), SIGINT);
        m_kill.start(2000);
    }
}

void Framare::cancel()
{
    for (Job &job : m_jobs)
        if (job.state == "Queued" || job.state == "Preparing") job.state = "Cancelled";
    emit jobsChanged();
    if (!m_operation.isEmpty()) { m_cancelled = true; interrupt(); }
}

void Framare::finishProcess(bool success)
{
    if (m_operation.isEmpty()) return;
    m_deadline.stop(); m_kill.stop(); drainOutput();
    const QString operation = m_operation;
    m_operation.clear();
    if (operation == "pair") {
        const bool valid = loadConfig();
        if (success && valid && m_config.value("frames").toObject().contains(m_pairName)) {
            m_config.insert("client_name", m_settings->value("senderName", "Framare").toString());
            saveConfig();
        } else if (valid) setError(m_timedOut ? tr("Pairing timed out.") :
                                   (m_cancelled ? tr("Pairing cancelled.") :
                                    tr("Pairing failed. Check the code, network and frame.")));
    } else if (m_current >= 0) {
        Job &job = m_jobs[m_current];
        if (success) { job.state = "Sent"; job.message = tr("Accepted by the frame."); }
        else if (m_timedOut) { job.state = "Failed"; job.message = tr("The transfer timed out. Receipt is unknown."); }
        else if (m_cancelled) { job.state = "Cancelled"; job.message = tr("Transfer cancelled. Receipt is unknown."); }
        else { job.state = "Failed"; job.message = tr("Transfer failed. Check the network, frame and pairing."); }
        m_current = -1; emit jobsChanged();
    }
    emit busyChanged(); runNext();
}

void Framare::clear()
{
    if (busy()) return;
    for (const Job &job : m_jobs) if (!job.cached.isEmpty()) QFile::remove(job.cached);
    m_jobs.clear(); emit jobsChanged();
}
