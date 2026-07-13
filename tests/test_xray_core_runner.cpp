#include "sys/XrayCoreRunner.hpp"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using NekoGui_sys::XrayCoreRunner;

class TestXrayCoreRunner : public QObject {
    Q_OBJECT
private slots:
    void missingBinary();
    void version();
    void configChecks();
    void timeout();
    void spacesInPaths();
};

static QString fakePath() { return QString::fromLocal8Bit(qgetenv("FAKE_XRAY_PATH")); }
static QString fixture(const QString &name) { return QString::fromLocal8Bit(qgetenv("XRAY_FIXTURE_DIR")) + QDir::separator() + name; }

void TestXrayCoreRunner::missingBinary() {
    XrayCoreRunner runner(fixture("missing-xray"));
    QVERIFY(!runner.binaryExists());
    QVERIFY(!runner.binaryExecutable());
    const auto result = runner.getVersion(1000);
    QVERIFY(!result.started);
    QVERIFY(!result.startError.isEmpty());
}

void TestXrayCoreRunner::version() {
    XrayCoreRunner runner(fakePath());
    const auto result = runner.getVersion(3000);
    QVERIFY(result.started);
    QCOMPARE(result.exitCode, 0);
    QVERIFY(result.stderrData.isEmpty());
    QVERIFY(result.stdoutData.contains("Penetrates Everything"));
    QCOMPARE(XrayCoreRunner::extractVersion(result.stdoutData), QStringLiteral("25.6.8"));
    QCOMPARE(XrayCoreRunner::extractVersion("Project X version v1.8.23 custom"), QStringLiteral("1.8.23"));
}

void TestXrayCoreRunner::configChecks() {
    XrayCoreRunner runner(fakePath());
    auto ok = runner.testConfig(fixture("valid config.json"), 3000);
    QCOMPARE(ok.exitCode, 0);
    QVERIFY(ok.stdoutData.contains("Configuration OK"));
    QVERIFY(ok.stderrData.contains("stderr notice"));

    auto bad = runner.testConfig(fixture("invalid config.json"), 3000);
    QVERIFY(bad.exitCode != 0);
    QVERIFY(bad.stdoutData.contains("checking config"));
    QVERIFY(bad.stderrData.contains("invalid character"));
}

void TestXrayCoreRunner::timeout() {
    XrayCoreRunner runner(fakePath());
    const auto result = runner.run({"--hang"}, 200);
    QVERIFY(result.timedOut);
    QVERIFY(result.started);
    QVERIFY(result.exitCode != 0 || result.exitStatus == QProcess::CrashExit);
    QCOMPARE(result.finalState, QProcess::NotRunning);
}

void TestXrayCoreRunner::spacesInPaths() {
    QTemporaryDir tempDir(QDir::tempPath() + "/xray runner spaces.XXXXXX");
    QVERIFY(tempDir.isValid());
    const QString config = tempDir.path() + "/safe config.json";
    QFile f(config);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("{\"log\":{\"loglevel\":\"warning\"}}");
    f.close();
    XrayCoreRunner runner(fakePath());
    const auto result = runner.testConfig(config, 3000);
    QCOMPARE(result.exitCode, 0);
}

QTEST_MAIN(TestXrayCoreRunner)
#include "test_xray_core_runner.moc"
