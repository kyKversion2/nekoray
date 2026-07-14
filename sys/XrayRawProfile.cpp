#include "XrayRawProfile.hpp"
#include "db/ProxyEntity.hpp"
#include "fmt/CustomBean.hpp"

namespace NekoGui_sys {

const QString XrayRawProfileCoreId = QStringLiteral("xray-raw");

bool IsXrayRawProfile(const std::shared_ptr<NekoGui::ProxyEntity> &ent) {
    if (ent == nullptr || ent->type != QStringLiteral("custom")) return false;
    auto bean = ent->CustomBean();
    return bean != nullptr && bean->core == XrayRawProfileCoreId;
}

bool ShouldCommitXrayStart(const void *currentSession, const void *candidateSession, int currentGeneration, int candidateGeneration, bool sessionRunning) {
    return currentSession != nullptr && currentSession == candidateSession && currentGeneration == candidateGeneration && sessionRunning;
}

} // namespace NekoGui_sys
