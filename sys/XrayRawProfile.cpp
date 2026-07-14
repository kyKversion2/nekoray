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

bool IsCurrentXraySession(const QObject *currentSession, const QObject *candidateSession, int currentGeneration, int candidateGeneration) {
    return currentSession != nullptr && currentSession == candidateSession && currentGeneration == candidateGeneration;
}

bool ShouldCommitXrayStart(const QObject *currentSession, const QObject *candidateSession, int currentGeneration, int candidateGeneration, bool sessionRunning) {
    return IsCurrentXraySession(currentSession, candidateSession, currentGeneration, candidateGeneration) && sessionRunning;
}

bool ShouldCommitXrayStop(const QObject *currentSession, const QObject *candidateSession, int currentGeneration, int candidateGeneration, bool sessionRunning) {
    return IsCurrentXraySession(currentSession, candidateSession, currentGeneration, candidateGeneration) && !sessionRunning;
}

} // namespace NekoGui_sys
