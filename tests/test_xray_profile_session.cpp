#include "sys/XrayProfileSession.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using NekoGui_sys::XrayProfileSession;

static QByteArray data(const QSignalSpy &spy) { QByteArray out; for (const auto &args: spy) out += args.at(0).toByteArray(); return out; }

class TestXrayProfileSession : public QObject {
    Q_OBJECT
private:
    QString fakePath() const { return QString::fromLocal8Bit(qgetenv("FAKE_XRAY_PATH")); }
private slots:
    void initTestCase() { qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus"); }
    void missingBinaryPath() { XrayProfileSession s(""); QVERIFY(!s.start("{}").ok); QVERIFY(!s.isRunning()); }
    void missingBinary() { XrayProfileSession s("/missing/fake xray"); auto r=s.start("{}"); QVERIFY(!r.ok); QVERIFY(!QFileInfo(s.configPathForTesting()).exists()); }
    void invalidJsonSyntax() { XrayProfileSession s(fakePath()); QVERIFY(!s.start("{").ok); }
    void arrayRootRejected() { XrayProfileSession s(fakePath()); QVERIFY(!s.start("[]").ok); }
    void emptyJsonRejected() { XrayProfileSession s(fakePath()); QVERIFY(!s.start("").ok); }
    void validRawObjectStartsAndStops() { XrayProfileSession s(fakePath()); QVERIFY(s.start("{}").ok); QVERIFY(QFileInfo(s.configPathForTesting()).exists()); QVERIFY(s.stop().ok); QVERIFY(!QFileInfo(s.configPathForTesting()).exists()); }
    void validationFailureCleansTemp() { XrayProfileSession s(fakePath()); auto r=s.start("{\"invalid\":true}"); QVERIFY(!r.ok); QVERIFY(!QFileInfo(s.configPathForTesting()).exists()); }
    void rawUnknownFieldPreserved() { XrayProfileSession s(fakePath()); QSignalSpy spy(&s,&XrayProfileSession::stdoutReceived); QByteArray raw="{\"futureUnknownField\":{\"nested\":[1,2,3]}}"; QVERIFY(s.start(raw).ok); QTRY_VERIFY_WITH_TIMEOUT(data(spy).contains("raw field preserved"), 3000); QVERIFY(s.stop().ok); }
    void noNormalization() { XrayProfileSession s(fakePath()); QByteArray raw="{  \n \"futureUnknownField\":{\"nested\":[1,2,3]}, \"n\":1.2300 }"; QVERIFY(s.start(raw).ok); QFile f(s.configPathForTesting()); QVERIFY(f.open(QIODevice::ReadOnly)); QCOMPARE(f.readAll(), raw); QVERIFY(s.stop().ok); }
    void configPathWithSpaces() { QTemporaryDir d(QDir::tempPath()+"/xray config spaces XXXXXX"); QVERIFY(d.isValid()); XrayProfileSession s(fakePath()); QVERIFY(s.start("{\"print-config-path-for-test\":true}").ok); QVERIFY(s.stop().ok); }
    void binaryPathWithSpaces() { QTemporaryDir d(QDir::tempPath()+"/xray bin spaces XXXXXX"); QVERIFY(d.isValid()); QString p=d.path()+"/fake xray"; QVERIFY(QFile::copy(fakePath(), p)); QVERIFY(QFile::setPermissions(p, QFile::permissions(p)|QFileDevice::ExeOwner|QFileDevice::ExeUser|QFileDevice::ExeGroup|QFileDevice::ExeOther)); XrayProfileSession s(p); QVERIFY(s.start("{}").ok); QVERIFY(s.stop().ok); }
    void tempExistsWhileActiveRemovedAfterStop() { XrayProfileSession s(fakePath()); QVERIFY(s.start("{}").ok); QString p=s.configPathForTesting(); QVERIFY(QFileInfo(p).exists()); QVERIFY(s.stop().ok); QVERIFY(!QFileInfo(p).exists()); }
    void tempRemovedAfterCrash() { XrayProfileSession s(fakePath()); QSignalSpy crash(&s,&XrayProfileSession::crashed); QVERIFY(s.start("{\"mode\":\"crash\"}").ok); QString p=s.configPathForTesting(); QVERIFY(crash.wait(3000)); QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo(p).exists(), 1000); }
    void destructorCleanup() { QString p; { XrayProfileSession s(fakePath()); QVERIFY(s.start("{}").ok); p=s.configPathForTesting(); QVERIFY(QFileInfo(p).exists()); } QVERIFY(!QFileInfo(p).exists()); }
    void stdoutForwarding() { XrayProfileSession s(fakePath()); QSignalSpy spy(&s,&XrayProfileSession::stdoutReceived); QVERIFY(s.start("{}").ok); QTRY_VERIFY_WITH_TIMEOUT(data(spy).contains("runtime ready"),3000); QVERIFY(s.stop().ok); }
    void stderrForwarding() { XrayProfileSession s(fakePath()); QSignalSpy spy(&s,&XrayProfileSession::stderrReceived); QVERIFY(s.start("{}").ok); QTRY_VERIFY_WITH_TIMEOUT(data(spy).contains("runtime stderr"),3000); QVERIFY(s.stop().ok); }
    void crashForwarding() { XrayProfileSession s(fakePath()); QSignalSpy spy(&s,&XrayProfileSession::crashed); QVERIFY(s.start("{\"mode\":\"crash\"}").ok); QVERIFY(spy.wait(3000)); }
    void restartCycle() { XrayProfileSession s(fakePath()); for(int i=0;i<2;i++){ QVERIFY(s.start("{}").ok); QVERIFY(s.stop().ok); } }
};

QTEST_MAIN(TestXrayProfileSession)
#include "test_xray_profile_session.moc"
