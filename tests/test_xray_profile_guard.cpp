#include "db/ConfigBuilder.hpp"
#include "db/Database.hpp"
#include "fmt/CustomBean.hpp"
#include "sys/XrayRawProfile.hpp"

#include <QApplication>
#include <QTest>

class TestXrayProfileGuard : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        NekoGui::dataStore = new NekoGui::DataStore();
        NekoGui::profileManager = new NekoGui::ProfileManager();
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
    void buildConfigRejectsXrayRaw() {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("custom");
        ent->CustomBean()->core = NekoGui_sys::XrayRawProfileCoreId;
        ent->CustomBean()->config_simple = "{}";
        auto result = NekoGui::BuildConfig(ent, false, false);
        QCOMPARE(result->error, QStringLiteral("Raw Xray profile must be started by the Xray runtime"));
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
