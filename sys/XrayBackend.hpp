#pragma once

#include "sys/XrayCoreRunner.hpp"

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QString>

namespace NekoGui_sys {

class XrayBackend : public QObject {
    Q_OBJECT
public:
    enum class State { Stopped, Validating, Starting, Running, Stopping, Crashed };

    struct OperationResult {
        bool ok = false;
        QString error;
        XrayCoreRunner::RunResult runnerResult;
    };

    explicit XrayBackend(QString binaryPath, QObject *parent = nullptr);
    ~XrayBackend() override;

    QString binaryPath() const;
    State state() const;
    OperationResult validateBinary() const;
    XrayCoreRunner::RunResult getVersion(int timeoutMs = 5000) const;
    OperationResult validateConfig(const QString &configPath, int timeoutMs = 5000) const;
    OperationResult start(const QString &configPath, int validationTimeoutMs = 5000);
    OperationResult stop(int terminateTimeoutMs = 3000, int killTimeoutMs = 1000);
    bool isRunning() const;
    qint64 processId() const;
    QString lastStartError() const;

signals:
    void stdoutReceived(const QByteArray &data);
    void stderrReceived(const QByteArray &data);
    void started();
    void stopped(int exitCode, QProcess::ExitStatus exitStatus);
    void crashed(int exitCode, QProcess::ExitStatus exitStatus);
    void stateChanged(NekoGui_sys::XrayBackend::State state);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onStarted();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onErrorOccurred(QProcess::ProcessError error);

private:
    void setState(State state);
    OperationResult makeError(QString error) const;

    QString binaryPath_;
    XrayCoreRunner runner_;
    QProcess process_;
    State state_ = State::Stopped;
    bool stopRequested_ = false;
    QString lastStartError_;
};

} // namespace NekoGui_sys

Q_DECLARE_METATYPE(NekoGui_sys::XrayBackend::State)
