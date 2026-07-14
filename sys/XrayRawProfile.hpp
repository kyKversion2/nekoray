#pragma once

#include <memory>
#include <QString>

namespace NekoGui { class ProxyEntity; }

namespace NekoGui_sys {

extern const QString XrayRawProfileCoreId;

bool IsXrayRawProfile(const std::shared_ptr<NekoGui::ProxyEntity> &ent);

} // namespace NekoGui_sys
