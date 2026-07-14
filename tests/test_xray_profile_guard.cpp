#include "db/ConfigBuilder.hpp"
#include "db/Database.hpp"
#include "fmt/CustomBean.hpp"
#include "fmt/ChainBean.hpp"
#include "fmt/SocksHttpBean.hpp"
#include "sys/XrayRawProfile.hpp"

#include <QApplication>
#include <QTest>

class TestXrayProfileGuard : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        NekoGui::dataStore = new NekoGui::DataStore();
        NekoGui::profileManager = new NekoGui::ProfileManager();
        auto group = NekoGui::ProfileManager::NewGroup();
        QVERIFY(NekoGui::profileManager->AddGroup(group));
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
        int session;
        int otherSession;
        QVERIFY(NekoGui_sys::ShouldCommitXrayStart(&session, &session, generation, generation, true));
        QVERIFY(!NekoGui_sys::ShouldCommitXrayStart(&session, &otherSession, generation, generation, true));
        QVERIFY(!NekoGui_sys::ShouldCommitXrayStart(&session, &session, generation, otherGeneration, true));
        QVERIFY(!NekoGui_sys::ShouldCommitXrayStart(&session, &session, generation, generation, false));
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
