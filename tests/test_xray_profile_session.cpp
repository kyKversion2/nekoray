#include "sys/XrayProfileSession.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using NekoGui_sys::XrayProfileSession;

static QString fakePath() { return QString::fromLocal8Bit(qgetenv("FAKE_XRAY_PATH")); }
static QByteArray data(QSignalSpy &spy){ QByteArray all; for (const auto &args: spy) all += args.at(0).toByteArray(); return all; }
static QString tempTemplate(const QTemporaryDir &d) { return d.path()+QStringLiteral("/nekoray-xray-raw-XXXXXX.json"); }

class TestXrayProfileSession : public QObject {
    Q_OBJECT
private slots:
    void missingBinaryPath() { XrayProfileSession s(""); QVERIFY(!s.start("{}").ok); QVERIFY(!s.isRunning()); }
    void missingBinary() { XrayProfileSession s("/missing/fake xray"); auto r=s.start("{}"); QVERIFY(!r.ok); QVERIFY(!QFileInfo(s.configPathForTesting()).exists()); }
    void invalidJsonSyntax() { XrayProfileSession s(fakePath()); QVERIFY(!s.start("{").ok); }
    void arrayRootRejected() { XrayProfileSession s(fakePath()); QVERIFY(!s.start("[]").ok); }
    void emptyJsonRejected() { XrayProfileSession s(fakePath()); QVERIFY(!s.start("").ok); }
    void validRawObjectStartsAndStops() { XrayProfileSession s(fakePath()); QVERIFY(s.start("{}").ok); QVERIFY(QFileInfo(s.configPathForTesting()).exists()); QVERIFY(s.stop().ok); QVERIFY(!QFileInfo(s.configPathForTesting()).exists()); }
    void validationFailureCleansTemp() { XrayProfileSession s(fakePath()); auto r=s.start("{\"invalid\":true}"); QVERIFY(!r.ok); QVERIFY(!QFileInfo(s.configPathForTesting()).exists()); }
    void rawUnknownFieldPreserved() { XrayProfileSession s(fakePath()); QSignalSpy spy(&s,&XrayProfileSession::stdoutReceived); QByteArray raw="{\"futureUnknownField\":{\"nested\":[1,2,3]}}"; QVERIFY(s.start(raw).ok); QTRY_VERIFY_WITH_TIMEOUT(data(spy).contains("raw field preserved"), 3000); QVERIFY(s.stop().ok); }
    void rawBytePassThrough() { XrayProfileSession s(fakePath()); QByteArray raw="{  \n \"futureUnknownField\":{\"nested\":[1,2,3]}, \"n\":1.2300 }"; QVERIFY(s.start(raw).ok); QFile f(s.configPathForTesting()); QVERIFY(f.open(QIODevice::ReadOnly)); QCOMPARE(f.readAll(), raw); QVERIFY(s.stop().ok); }
    void configPathWithSpaces() { QTemporaryDir d(QDir::tempPath()+"/xray config spaces XXXXXX"); QVERIFY(d.isValid()); XrayProfileSession s(fakePath(), tempTemplate(d)); QSignalSpy spy(&s,&XrayProfileSession::stdoutReceived); QVERIFY(s.start("{\"print-config-path-for-test\":true}").ok); const QString path=s.configPathForTesting(); QVERIFY(path.contains(' ')); QVERIFY(QFileInfo(path).exists()); QTRY_VERIFY_WITH_TIMEOUT(QString::fromUtf8(data(spy)).contains(path),3000); QVERIFY(s.stop().ok); }
    void binaryPathWithSpaces() { QTemporaryDir d(QDir::tempPath()+"/xray bin spaces XXXXXX"); QVERIFY(d.isValid()); const QString name=QFileInfo(fakePath()).fileName(); QVERIFY(!name.isEmpty()); QString p=d.path()+"/"+name; QVERIFY(QFile::copy(fakePath(), p)); QVERIFY(QFile::setPermissions(p, QFile::permissions(p)|QFileDevice::ExeOwner|QFileDevice::ExeUser|QFileDevice::ExeGroup|QFileDevice::ExeOther)); QVERIFY(d.path().contains(' '));
#ifdef Q_OS_WIN
        QVERIFY(p.endsWith(".exe", Qt::CaseInsensitive));
#endif
        XrayProfileSession s(p); QVERIFY(s.start("{}").ok); QVERIFY(s.stop().ok); }
    void doubleStartPreservesActiveConfig() { XrayProfileSession s(fakePath()); QVERIFY(s.start("{}").ok); const QString p=s.configPathForTesting(); QVERIFY(QFileInfo(p).exists()); auto r=s.start("{\"n\":2}"); QVERIFY(!r.ok); QCOMPARE(s.configPathForTesting(), p); QVERIFY(QFileInfo(p).exists()); QVERIFY(s.stop().ok); }
    void stopFailurePreservesConfigWhileRunning() { XrayProfileSession s(fakePath()); QVERIFY(s.start("{}").ok); const QString p=s.configPathForTesting(); auto r=s.stop(0,0); if (!r.ok && s.isRunning()) QVERIFY(QFileInfo(p).exists()); s.stop(3000,1000); }
    void tempExistsWhileActiveRemovedAfterStop() { XrayProfileSession s(fakePath()); QVERIFY(s.start("{}").ok); QString p=s.configPathForTesting(); QVERIFY(QFileInfo(p).exists()); QVERIFY(s.stop().ok); QVERIFY(!QFileInfo(p).exists()); }
    void immediateCrashCleanup() { XrayProfileSession s(fakePath()); QSignalSpy crash(&s,&XrayProfileSession::crashed); QVERIFY(s.start("{\"mode\":\"immediate-crash\"}").ok); QString p=s.configPathForTesting(); QVERIFY(crash.wait(3000)); QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo(p).exists(), 1000); }
    void restartAfterCrash() { XrayProfileSession s(fakePath()); QSignalSpy crash(&s,&XrayProfileSession::crashed); QVERIFY(s.start("{\"mode\":\"immediate-crash\"}").ok); QVERIFY(crash.wait(3000)); QVERIFY(s.start("{}").ok); QVERIFY(s.stop().ok); }
    void tempRemovedAfterCrash() { XrayProfileSession s(fakePath()); QSignalSpy crash(&s,&XrayProfileSession::crashed); QVERIFY(s.start("{\"mode\":\"crash\"}").ok); QString p=s.configPathForTesting(); QVERIFY(crash.wait(3000)); QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo(p).exists(), 1000); }
    void destructorCleanup() { QString p; { XrayProfileSession s(fakePath()); QVERIFY(s.start("{}").ok); p=s.configPathForTesting(); QVERIFY(QFileInfo(p).exists()); } QVERIFY(!QFileInfo(p).exists()); }
    void stdoutForwarding() { XrayProfileSession s(fakePath()); QSignalSpy spy(&s,&XrayProfileSession::stdoutReceived); QVERIFY(s.start("{}").ok); QTRY_VERIFY_WITH_TIMEOUT(data(spy).contains("runtime ready"),3000); QVERIFY(s.stop().ok); }
    void stderrForwarding() { XrayProfileSession s(fakePath()); QSignalSpy spy(&s,&XrayProfileSession::stderrReceived); QVERIFY(s.start("{}").ok); QTRY_VERIFY_WITH_TIMEOUT(data(spy).contains("runtime stderr"),3000); QVERIFY(s.stop().ok); }
    void crashForwarding() { XrayProfileSession s(fakePath()); QSignalSpy spy(&s,&XrayProfileSession::crashed); QVERIFY(s.start("{\"mode\":\"crash\"}").ok); QVERIFY(spy.wait(3000)); }
    void restartCycle() { XrayProfileSession s(fakePath()); for(int i=0;i<2;i++){ QVERIFY(s.start("{}").ok); QVERIFY(s.stop().ok); } }
};

QTEST_MAIN(TestXrayProfileSession)
#include "test_xray_profile_session.moc"
