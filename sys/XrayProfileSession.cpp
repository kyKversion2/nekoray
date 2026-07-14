#include "XrayProfileSession.hpp"

#include <QDir>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QThread>

namespace NekoGui_sys {

XrayProfileSession::XrayProfileSession(QString binaryPath, QObject *parent)
    : QObject(parent), binaryPath_(std::move(binaryPath)), backend_(new XrayBackend(binaryPath_, this)) {
    connect(backend_.data(), &XrayBackend::stdoutReceived, this, &XrayProfileSession::stdoutReceived);
    connect(backend_.data(), &XrayBackend::stderrReceived, this, &XrayProfileSession::stderrReceived);
    connect(backend_.data(), &XrayBackend::started, this, &XrayProfileSession::started);
    connect(backend_.data(), &XrayBackend::stopped, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        cleanupTempFile();
        emit stopped(exitCode, exitStatus);
    });
    connect(backend_.data(), &XrayBackend::crashed, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        cleanupTempFile();
        emit crashed(exitCode, exitStatus);
    });
}

XrayProfileSession::~XrayProfileSession() {
    if (backend_ && backend_->isRunning()) backend_->stop(1000, 1000);
    cleanupTempFile();
}

XrayProfileSession::StartResult XrayProfileSession::start(const QByteArray &rawJsonUtf8, int validationTimeoutMs) {
    Q_ASSERT(thread() == QThread::currentThread());
    if (binaryPath_.trimmed().isEmpty()) return makeError(QStringLiteral("Xray binary path is empty. Configure Extra Core 'xray' first."));
    if (rawJsonUtf8.trimmed().isEmpty()) return makeError(QStringLiteral("Raw Xray config is empty"));

    QJsonParseError parseError{};
    const auto doc = QJsonDocument::fromJson(rawJsonUtf8, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return makeError(QStringLiteral("Raw Xray config JSON syntax error: %1").arg(parseError.errorString()));
    }
    if (!doc.isObject()) return makeError(QStringLiteral("Raw Xray config JSON root must be an object"));

    cleanupTempFile();
    configFile_.reset(new QTemporaryFile(QDir::tempPath() + QStringLiteral("/nekoray-xray-raw-XXXXXX.json")));
    configFile_->setAutoRemove(true);
    if (!configFile_->open()) {
        const auto error = configFile_->errorString();
        cleanupTempFile();
        return makeError(error);
    }
    if (configFile_->write(rawJsonUtf8) != rawJsonUtf8.size()) {
        const auto error = configFile_->errorString();
        cleanupTempFile();
        return makeError(error.isEmpty() ? QStringLiteral("Failed to write raw Xray config") : error);
    }
    configFile_->flush();

    auto started = backend_->start(configFile_->fileName(), validationTimeoutMs);
    if (!started.ok) {
        const auto error = started.error;
        cleanupTempFile();
        return {false, error, started};
    }
    return {true, {}, started};
}

XrayBackend::OperationResult XrayProfileSession::stop(int terminateTimeoutMs, int killTimeoutMs) {
    Q_ASSERT(thread() == QThread::currentThread());
    auto result = backend_->stop(terminateTimeoutMs, killTimeoutMs);
    cleanupTempFile();
    return result;
}

bool XrayProfileSession::isRunning() const { return backend_ && backend_->isRunning(); }
QString XrayProfileSession::configPathForTesting() const { return configFile_ ? configFile_->fileName() : QString(); }
XrayBackend *XrayProfileSession::backend() const { return backend_.data(); }
void XrayProfileSession::cleanupTempFile() { configFile_.reset(); }
XrayProfileSession::StartResult XrayProfileSession::makeError(const QString &error) { cleanupTempFile(); return {false, error, {false, error, {}}}; }

} // namespace NekoGui_sys
