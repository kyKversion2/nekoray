#include "db/ConfigBuilder.hpp"
#include "db/Database.hpp"
#include "fmt/CustomBean.hpp"
#include "fmt/ChainBean.hpp"
#include "fmt/SocksHttpBean.hpp"
#include "sys/XrayRawProfile.hpp"

#include <QApplication>
#include <QDir>
#include <QTest>
#include <QTemporaryDir>

#include <memory>

class TestXrayProfileGuard : public QObject {
    Q_OBJECT
private:
    QString originalCurrentPath;
    std::unique_ptr<QTemporaryDir> testDataDir;

private slots:
    void initTestCase() {
        originalCurrentPath = QDir::currentPath();
        testDataDir = std::make_unique<QTemporaryDir>();
        QVERIFY(testDataDir->isValid());
        QVERIFY(QDir::setCurrent(testDataDir->path()));
        QVERIFY(QDir().mkpath(QStringLiteral("groups")));
        QVERIFY(QDir().mkpath(QStringLiteral("profiles")));

        NekoGui::dataStore = new NekoGui::DataStore();
        NekoGui::dataStore->routing = std::make_unique<NekoGui::Routing>();
        NekoGui::profileManager = new NekoGui::ProfileManager();
        auto group = NekoGui::ProfileManager::NewGroup();
        QVERIFY(NekoGui::profileManager->AddGroup(group));
    }
    void cleanupTestCase() {
        delete NekoGui::profileManager;
        NekoGui::profileManager = nullptr;
        delete NekoGui::dataStore;
        NekoGui::dataStore = nullptr;
        QVERIFY(QDir::setCurrent(originalCurrentPath));
        testDataDir.reset();
    }
    void normalCustomNotXray() {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("custom");
        ent->CustomBean()->core = "custom";
        QVERIFY(!NekoGui_sys::IsXrayRawProfile(ent));
    }
    void internalFullNotXray() {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("custom");
        ent->CustomBean()->core = "internal-full";
        QVERIFY(!NekoGui_sys::IsXrayRawProfile(ent));
    }
    void xrayRawDetected() {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("custom");
        ent->CustomBean()->core = NekoGui_sys::XrayRawProfileCoreId;
        QVERIFY(NekoGui_sys::IsXrayRawProfile(ent));
    }
    void xrayRawRejectedInChain() {
        auto raw = NekoGui::ProfileManager::NewProxyEntity("custom");
        raw->CustomBean()->core = NekoGui_sys::XrayRawProfileCoreId;
        QVERIFY(NekoGui::profileManager->AddProfile(raw, 0));
        auto chain = NekoGui::ProfileManager::NewProxyEntity("chain");
        chain->ChainBean()->list = {raw->id};
        QVERIFY(NekoGui::profileManager->AddProfile(chain, 0));
        auto result = NekoGui::BuildConfig(chain, false, false);
        QCOMPARE(result->error, QStringLiteral("Raw Xray profile cannot be used in sing-box chains"));
    }
    void buildConfigRejectsXrayRaw() {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("custom");
        ent->CustomBean()->core = NekoGui_sys::XrayRawProfileCoreId;
        ent->CustomBean()->config_simple = "{}";
        auto result = NekoGui::BuildConfig(ent, false, false);
        QCOMPARE(result->error, QStringLiteral("Raw Xray profile must be started by the Xray runtime"));
    }
    void ordinaryGeneratedSocksBuildConfigStillWorks() {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("socks");
        ent->SocksHTTPBean()->socks_http_type = NekoGui_fmt::SocksHttpBean::type_Socks5;
        ent->bean->serverAddress = "127.0.0.1";
        ent->bean->serverPort = 1080;
        QVERIFY(NekoGui::profileManager->AddProfile(ent, 0));
        auto result = NekoGui::BuildConfig(ent, false, false);
        QVERIFY2(result->error.isEmpty(), qPrintable(result->error));
        QVERIFY(result->coreConfig.contains("outbounds"));
    }
    void staleCrashedStartCommitRejected() {
        int generation = 7;
        int otherGeneration = 8;
        QObject session;
        QObject otherSession;
        QVERIFY(NekoGui_sys::ShouldCommitXrayStart(&session, &session, generation, generation, true));
        QVERIFY(!NekoGui_sys::ShouldCommitXrayStart(&session, &otherSession, generation, generation, true));
        QVERIFY(!NekoGui_sys::ShouldCommitXrayStart(&session, &session, generation, otherGeneration, true));
        QVERIFY(!NekoGui_sys::ShouldCommitXrayStart(&session, &session, generation, generation, false));
    }
    void stoppedCurrentSessionCommitsStoppedState() {
        int generation = 7;
        int otherGeneration = 8;
        QObject session;
        QObject otherSession;
        QVERIFY(NekoGui_sys::IsCurrentXraySession(&session, &session, generation, generation));
        QVERIFY(!NekoGui_sys::IsCurrentXraySession(&session, &otherSession, generation, generation));
        QVERIFY(!NekoGui_sys::IsCurrentXraySession(&session, &session, generation, otherGeneration));
        QVERIFY(NekoGui_sys::ShouldCommitXrayStop(&session, &session, generation, generation, false));
        QVERIFY(!NekoGui_sys::ShouldCommitXrayStop(&session, &session, generation, generation, true));
        QVERIFY(!NekoGui_sys::ShouldCommitXrayStop(&session, &otherSession, generation, generation, false));
        QVERIFY(!NekoGui_sys::ShouldCommitXrayStop(&session, &session, generation, otherGeneration, false));
    }
    void ordinaryInternalFullStillBuilds() {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("custom");
        ent->CustomBean()->core = "internal-full";
        ent->CustomBean()->config_simple = "{\"inbounds\":[],\"outbounds\":[]}";
        auto result = NekoGui::BuildConfig(ent, false, false);
        QVERIFY(result->error.isEmpty());
        QVERIFY(result->coreConfig.contains("inbounds"));
    }
};

QTEST_MAIN(TestXrayProfileGuard)
#include "test_xray_profile_guard.moc"
