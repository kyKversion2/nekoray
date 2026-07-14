#pragma once

#include <memory>
#include <QString>

class QObject;
namespace NekoGui { class ProxyEntity; }

namespace NekoGui_sys {

extern const QString XrayRawProfileCoreId;

bool IsXrayRawProfile(const std::shared_ptr<NekoGui::ProxyEntity> &ent);
bool IsCurrentXraySession(const QObject *currentSession, const QObject *candidateSession, int currentGeneration, int candidateGeneration);
bool ShouldCommitXrayStart(const QObject *currentSession, const QObject *candidateSession, int currentGeneration, int candidateGeneration, bool sessionRunning);
bool ShouldCommitXrayStop(const QObject *currentSession, const QObject *candidateSession, int currentGeneration, int candidateGeneration, bool sessionRunning);

} // namespace NekoGui_sys
