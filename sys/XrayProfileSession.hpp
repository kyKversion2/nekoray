#pragma once

#include "sys/XrayBackend.hpp"

#include <QObject>
#include <QScopedPointer>
#include <QTemporaryFile>

namespace NekoGui_sys {

class XrayProfileSession : public QObject {
    Q_OBJECT
public:
    struct StartResult {
        bool ok = false;
        QString error;
        XrayBackend::OperationResult backendResult;
    };

    explicit XrayProfileSession(QString binaryPath, QObject *parent = nullptr);
    XrayProfileSession(QString binaryPath, QString tempDirectoryTemplate, QObject *parent = nullptr);
    ~XrayProfileSession() override;

    StartResult start(const QByteArray &rawJsonUtf8, int validationTimeoutMs = 5000);
    XrayBackend::OperationResult stop(int terminateTimeoutMs = 3000, int killTimeoutMs = 1000);
    bool isRunning() const;
    QString configPathForTesting() const;
    XrayBackend *backend() const;

signals:
    void stdoutReceived(const QByteArray &data);
    void stderrReceived(const QByteArray &data);
    void started();
    void stopped(int exitCode, QProcess::ExitStatus exitStatus);
    void crashed(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void cleanupTempFile();
    StartResult makeError(const QString &error);

    QString binaryPath_;
    QString tempDirectoryTemplate_;
    QScopedPointer<XrayBackend> backend_;
    QScopedPointer<QTemporaryFile> configFile_;
};

} // namespace NekoGui_sys

Q_DECLARE_METATYPE(NekoGui_sys::XrayProfileSession::StartResult)
