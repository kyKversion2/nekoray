#include "XrayCoreRunner.hpp"

#include <QFileInfo>
#include <QRegularExpression>

#include <utility>

namespace NekoGui_sys {

XrayCoreRunner::XrayCoreRunner(QString binaryPath) : binaryPath_(std::move(binaryPath)) {}

QString XrayCoreRunner::binaryPath() const { return binaryPath_; }

bool XrayCoreRunner::binaryExists() const { return QFileInfo::exists(binaryPath_); }

bool XrayCoreRunner::binaryExecutable() const {
    const QFileInfo info(binaryPath_);
    return info.exists() && info.isFile() && info.isExecutable();
}

QString XrayCoreRunner::binaryValidationError() const {
    const QFileInfo info(binaryPath_);
    if (!info.exists()) return QStringLiteral("Xray binary does not exist");
    if (!info.isFile()) return QStringLiteral("Xray binary path is not a file");
    if (!info.isExecutable()) return QStringLiteral("Xray binary is not executable");
    return {};
}

XrayCoreRunner::RunResult XrayCoreRunner::run(const QStringList &arguments, int timeoutMs) const {
    RunResult result;
    const auto validationError = binaryValidationError();
    if (!validationError.isEmpty()) {
        result.startError = validationError;
        return result;
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(binaryPath_, arguments);

    if (!process.waitForStarted()) {
        result.startError = process.errorString();
        result.stdoutData = process.readAllStandardOutput();
        result.stderrData = process.readAllStandardError();
        return result;
    }

    result.started = true;
    if (!process.waitForFinished(timeoutMs)) {
        result.timedOut = true;
        process.terminate();
        if (!process.waitForFinished(1000)) {
            process.kill();
            process.waitForFinished(1000);
        }
    }

    result.stdoutData = process.readAllStandardOutput();
    result.stderrData = process.readAllStandardError();
    result.exitCode = process.exitCode();
    result.exitStatus = process.exitStatus();
    result.finalState = process.state();
    if (process.error() != QProcess::UnknownError && result.startError.isEmpty()) {
        result.startError = process.errorString();
    }
    return result;
}

XrayCoreRunner::RunResult XrayCoreRunner::getVersion(int timeoutMs) const {
    return run({QStringLiteral("version")}, timeoutMs);
}

XrayCoreRunner::RunResult XrayCoreRunner::testConfig(const QString &configPath, int timeoutMs) const {
    return run({QStringLiteral("run"), QStringLiteral("-test"), QStringLiteral("-config"), configPath}, timeoutMs);
}

QString XrayCoreRunner::extractVersion(const QByteArray &versionOutput) {
    const QString output = QString::fromUtf8(versionOutput);
    const QRegularExpression modern(QStringLiteral(R"(^\s*Xray\s+([0-9]+(?:\.[0-9A-Za-z-]+)+))"),
                                    QRegularExpression::MultilineOption);
    auto match = modern.match(output);
    if (match.hasMatch()) return match.captured(1);

    const QRegularExpression generic(QStringLiteral(R"((?:version\s*)?v?([0-9]+\.[0-9]+(?:\.[0-9A-Za-z-]+)*))"),
                                     QRegularExpression::CaseInsensitiveOption);
    match = generic.match(output);
    if (match.hasMatch()) return match.captured(1);
    return {};
}

} // namespace NekoGui_sys
