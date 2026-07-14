#include "sys/XrayBackend.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using NekoGui_sys::XrayBackend;

class TestXrayBackend : public QObject {
    Q_OBJECT
private:
    QString fakePath() const { return QString::fromLocal8Bit(qgetenv("FAKE_XRAY_PATH")); }
    QString fixtureDir() const { return QString::fromLocal8Bit(qgetenv("XRAY_FIXTURE_DIR")); }
    QString fixture(const QString &name) const { return QDir(fixtureDir()).filePath(name); }
    QString writeConfig(QTemporaryDir &dir, const QString &name, const QByteArray &data) const {
        const QString path = QDir(dir.path()).filePath(name);
        QFile f(path);
        QDir().mkpath(QFileInfo(path).path());
        Q_ASSERT(f.open(QIODevice::WriteOnly));
        Q_ASSERT(f.write(data) == qint64(data.size()));
        return path;
    }

private slots:
    void initTestCase();
    void missingBinary();
    void missingConfig();
    void invalidConfigDoesNotStart();
    void validConfigStartsAndStops();
    void secondStartRejected();
    void stdoutDelivery();
    void stderrDelivery();
    void crashDetection();
    void destructorCleanup();
    void spacesInPaths();
    void restartCycle();
};

void TestXrayBackend::initTestCase() {
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
}

void TestXrayBackend::missingBinary() {
    XrayBackend backend(fixture("missing-xray"));
    auto result = backend.start(fixture("valid config.json"));
    QVERIFY(!result.ok);
    QVERIFY(!result.error.isEmpty());
    QVERIFY(!backend.isRunning());
}

void TestXrayBackend::missingConfig() {
    XrayBackend backend(fakePath());
    auto result = backend.start(fixture("missing config.json"));
    QVERIFY(!result.ok);
    QCOMPARE(result.error, QStringLiteral("Xray config does not exist"));
    QVERIFY(!backend.isRunning());
}

void TestXrayBackend::invalidConfigDoesNotStart() {
    XrayBackend backend(fakePath());
    auto result = backend.start(fixture("invalid config.json"));
    QVERIFY(!result.ok);
    QCOMPARE(result.error, QStringLiteral("Xray config validation failed"));
    QVERIFY(!backend.isRunning());
}

void TestXrayBackend::validConfigStartsAndStops() {
    XrayBackend backend(fakePath());
    QSignalSpy started(&backend, &XrayBackend::started);
    QSignalSpy stopped(&backend, &XrayBackend::stopped);
    auto result = backend.start(fixture("valid config.json"));
    QVERIFY(result.ok);
    if (started.isEmpty()) QVERIFY(started.wait(1000));
    QVERIFY(backend.isRunning());
    QVERIFY(backend.processId() > 0);
    auto stop = backend.stop();
    QVERIFY(stop.ok);
    if (stopped.isEmpty()) QVERIFY(stopped.wait(1000));
    QVERIFY(!backend.isRunning());
}

void TestXrayBackend::secondStartRejected() {
    XrayBackend backend(fakePath());
    QVERIFY(backend.start(fixture("valid config.json")).ok);
    auto second = backend.start(fixture("valid config.json"));
    QVERIFY(!second.ok);
    QCOMPARE(second.error, QStringLiteral("Xray backend is already running"));
    QVERIFY(backend.stop().ok);
}

void TestXrayBackend::stdoutDelivery() {
    XrayBackend backend(fakePath());
    QSignalSpy stdoutSpy(&backend, &XrayBackend::stdoutReceived);
    QVERIFY(backend.start(fixture("valid config.json")).ok);
    QVERIFY(stdoutSpy.wait(2000));
    QByteArray combined;
    for (const auto &args : stdoutSpy) combined += args.at(0).toByteArray();
    QVERIFY(combined.contains("runtime ready"));
    QVERIFY(backend.stop().ok);
}

void TestXrayBackend::stderrDelivery() {
    XrayBackend backend(fakePath());
    QSignalSpy stderrSpy(&backend, &XrayBackend::stderrReceived);
    QVERIFY(backend.start(fixture("valid config.json")).ok);
    QVERIFY(stderrSpy.wait(2000));
    QByteArray combined;
    for (const auto &args : stderrSpy) combined += args.at(0).toByteArray();
    QVERIFY(combined.contains("runtime stderr"));
    QVERIFY(backend.stop().ok);
}

void TestXrayBackend::crashDetection() {
    QTemporaryDir dir;
    const QString config = writeConfig(dir, "crash.json", "{\"mode\":\"crash\"}");
    XrayBackend backend(fakePath());
    QSignalSpy crashed(&backend, &XrayBackend::crashed);
    QVERIFY(backend.start(config).ok);
    QVERIFY(crashed.wait(3000));
    QCOMPARE(crashed.takeFirst().at(0).toInt(), 42);
    QVERIFY(!backend.isRunning());
}

void TestXrayBackend::destructorCleanup() {
    qint64 pid = 0;
    {
        auto *backend = new XrayBackend(fakePath());
        QVERIFY(backend->start(fixture("valid config.json")).ok);
        QVERIFY(backend->isRunning());
        pid = backend->processId();
        QVERIFY(pid > 0);
        delete backend;
    }
    QVERIFY(pid > 0);
}

void TestXrayBackend::spacesInPaths() {
    QTemporaryDir dir;
    const QString binaryDir = QDir(dir.path()).filePath("bin with spaces");
    QVERIFY(QDir().mkpath(binaryDir));
    const QString copiedBinary = QDir(binaryDir).filePath("fake xray");
    QVERIFY(QFile::copy(fakePath(), copiedBinary));
    QVERIFY(QFile::setPermissions(copiedBinary, QFile::permissions(copiedBinary) | QFileDevice::ExeOwner | QFileDevice::ExeUser | QFileDevice::ExeGroup | QFileDevice::ExeOther));
    const QString config = writeConfig(dir, "config with spaces/valid config.json", "{}");
    XrayBackend backend(copiedBinary);
    QVERIFY(backend.start(config).ok);
    QVERIFY(backend.isRunning());
    QVERIFY(backend.stop().ok);
}

void TestXrayBackend::restartCycle() {
    XrayBackend backend(fakePath());
    QVERIFY(backend.start(fixture("valid config.json")).ok);
    QVERIFY(backend.stop().ok);
    QVERIFY(!backend.isRunning());
    QVERIFY(backend.start(fixture("valid config.json")).ok);
    QVERIFY(backend.stop().ok);
    QVERIFY(!backend.isRunning());
}

QTEST_MAIN(TestXrayBackend)
#include "test_xray_backend.moc"
