#include "XrayBackend.hpp"

#include <QFileInfo>

#include <utility>

namespace NekoGui_sys {

XrayBackend::XrayBackend(QString binaryPath, QObject *parent)
    : QObject(parent), binaryPath_(std::move(binaryPath)), runner_(binaryPath_) {
    process_.setProcessChannelMode(QProcess::SeparateChannels);
    connect(&process_, &QProcess::readyReadStandardOutput, this, &XrayBackend::onReadyReadStandardOutput);
    connect(&process_, &QProcess::readyReadStandardError, this, &XrayBackend::onReadyReadStandardError);
    connect(&process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &XrayBackend::onFinished);
    connect(&process_, &QProcess::started, this, &XrayBackend::onStarted);
    connect(&process_, &QProcess::errorOccurred, this, &XrayBackend::onErrorOccurred);
}

XrayBackend::~XrayBackend() {
    if (process_.state() != QProcess::NotRunning) {
        stop(1000, 1000);
    }
}

QString XrayBackend::binaryPath() const { return binaryPath_; }

XrayBackend::State XrayBackend::state() const { return state_; }

XrayBackend::OperationResult XrayBackend::validateBinary() const {
    const QString error = runner_.binaryValidationError();
    if (!error.isEmpty()) return makeError(error);
    return {true, {}, {}};
}

XrayCoreRunner::RunResult XrayBackend::getVersion(int timeoutMs) const { return runner_.getVersion(timeoutMs); }

XrayBackend::OperationResult XrayBackend::validateConfig(const QString &configPath, int timeoutMs) const {
    auto binary = validateBinary();
    if (!binary.ok) return binary;
    const QFileInfo configInfo(configPath);
    if (!configInfo.exists()) return makeError(QStringLiteral("Xray config does not exist"));
    if (!configInfo.isFile()) return makeError(QStringLiteral("Xray config path is not a file"));

    auto result = runner_.testConfig(configPath, timeoutMs);
    if (!result.started) return {false, result.startError, result};
    if (result.timedOut) return {false, QStringLiteral("Xray config validation timed out"), result};
    if (!result.terminationError.isEmpty()) return {false, result.terminationError, result};
    if (result.exitStatus != QProcess::NormalExit || result.exitCode != 0) {
        return {false, QStringLiteral("Xray config validation failed"), result};
    }
    return {true, {}, result};
}

XrayBackend::OperationResult XrayBackend::start(const QString &configPath, int validationTimeoutMs) {
    if (process_.state() != QProcess::NotRunning) {
        lastStartError_ = QStringLiteral("Xray backend process is already running");
        return makeError(lastStartError_);
    }
    if (state_ != State::Stopped && state_ != State::Crashed) {
        lastStartError_ = QStringLiteral("Xray backend cannot start from current lifecycle state");
        return makeError(lastStartError_);
    }

    setState(State::Validating);
    auto validation = validateConfig(configPath, validationTimeoutMs);
    if (!validation.ok) {
        lastStartError_ = validation.error;
        setState(State::Stopped);
        return validation;
    }

    stopRequested_ = false;
    lastStartError_.clear();
    setState(State::Starting);
    process_.setProgram(binaryPath_);
    process_.setArguments({QStringLiteral("run"), QStringLiteral("-config"), configPath});
    process_.start();
    if (!process_.waitForStarted(5000)) {
        lastStartError_ = process_.errorString();
        setState(State::Stopped);
        return makeError(lastStartError_);
    }
    return {true, {}, validation.runnerResult};
}

XrayBackend::OperationResult XrayBackend::stop(int terminateTimeoutMs, int killTimeoutMs) {
    if (process_.state() == QProcess::NotRunning) {
        if (state_ != State::Crashed) setState(State::Stopped);
        return {true, {}, {}};
    }
    stopRequested_ = true;
    setState(State::Stopping);
    process_.terminate();
    if (!process_.waitForFinished(terminateTimeoutMs)) {
        process_.kill();
        if (!process_.waitForFinished(killTimeoutMs)) {
            return makeError(QStringLiteral("Xray process did not stop after terminate and kill"));
        }
    }
    if (process_.state() != QProcess::NotRunning) {
        return makeError(QStringLiteral("Xray process is still running after stop"));
    }
    setState(State::Stopped);
    return {true, {}, {}};
}

bool XrayBackend::isRunning() const { return process_.state() != QProcess::NotRunning; }

qint64 XrayBackend::processId() const { return process_.state() == QProcess::NotRunning ? 0 : process_.processId(); }

QString XrayBackend::lastStartError() const { return lastStartError_; }

void XrayBackend::onReadyReadStandardOutput() { emit stdoutReceived(process_.readAllStandardOutput()); }

void XrayBackend::onReadyReadStandardError() { emit stderrReceived(process_.readAllStandardError()); }

void XrayBackend::onStarted() {
    setState(State::Running);
    emit started();
}

void XrayBackend::onFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    const bool requested = stopRequested_ || state_ == State::Stopping;
    stopRequested_ = false;
    if (requested) {
        setState(State::Stopped);
        emit stopped(exitCode, exitStatus);
    } else {
        setState(State::Crashed);
        emit crashed(exitCode, exitStatus);
    }
}

void XrayBackend::onErrorOccurred(QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) lastStartError_ = process_.errorString();
}

void XrayBackend::setState(State state) {
    if (state_ == state) return;
    state_ = state;
    emit stateChanged(state_);
}

XrayBackend::OperationResult XrayBackend::makeError(QString error) const { return {false, std::move(error), {}}; }

} // namespace NekoGui_sys
