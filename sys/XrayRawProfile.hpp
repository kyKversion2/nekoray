#pragma once

#include <memory>
#include <QString>

namespace NekoGui { class ProxyEntity; }

namespace NekoGui_sys {

extern const QString XrayRawProfileCoreId;

bool IsXrayRawProfile(const std::shared_ptr<NekoGui::ProxyEntity> &ent);
bool ShouldCommitXrayStart(const void *currentSession, const void *candidateSession, int currentGeneration, int candidateGeneration, bool sessionRunning);

} // namespace NekoGui_sys
