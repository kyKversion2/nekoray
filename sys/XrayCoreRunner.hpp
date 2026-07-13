#pragma once

#include <QProcess>
#include <QString>
#include <QStringList>

namespace NekoGui_sys {

class XrayCoreRunner {
public:
    struct RunResult {
        QByteArray stdoutData;
        QByteArray stderrData;
        int exitCode = -1;
        QProcess::ExitStatus exitStatus = QProcess::NormalExit;
        QString startError;
        bool timedOut = false;
        bool started = false;
        QProcess::ProcessState finalState = QProcess::NotRunning;
    };

    explicit XrayCoreRunner(QString binaryPath);

    QString binaryPath() const;
    bool binaryExists() const;
    bool binaryExecutable() const;
    QString binaryValidationError() const;

    RunResult run(const QStringList &arguments, int timeoutMs) const;
    RunResult getVersion(int timeoutMs) const;
    RunResult testConfig(const QString &configPath, int timeoutMs) const;

    static QString extractVersion(const QByteArray &versionOutput);

private:
    QString binaryPath_;
};

} // namespace NekoGui_sys
