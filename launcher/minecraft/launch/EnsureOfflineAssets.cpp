// SPDX-License-Identifier: GPL-3.0-only
#include "EnsureOfflineAssets.h"

#include <QFileInfo>

#include "Application.h"
#include "minecraft/AssetsUtils.h"
#include "minecraft/LaunchProfile.h"
#include "minecraft/PackProfile.h"
#include "minecraft/update/AssetUpdateTask.h"
#include "net/HttpMetaCache.h"

EnsureOfflineAssets::EnsureOfflineAssets(LaunchTask* parent, MinecraftInstance* instance) : LaunchStep(parent), m_instance(instance) {}

void EnsureOfflineAssets::executeTask()
{
    const auto profile = m_instance->getPackProfile()->getProfile();
    const auto assetInfo = profile->getMinecraftAssets();
    if (!assetInfo || assetInfo->id.isEmpty()) {
        emitSucceeded();
        return;
    }

    const QString indexName = assetInfo->id + ".json";
    const auto indexEntry = APPLICATION->metacache()->resolveEntry("asset_indexes", indexName);
    AssetsIndex index;
    bool missingAssets = !QFileInfo(indexEntry->getFullPath()).isFile() ||
                         !AssetsUtils::loadAssetsIndexJson(assetInfo->id, indexEntry->getFullPath(), index);

    if (!missingAssets) {
        for (const auto& asset : index.objects) {
            const QFileInfo objectFile(asset.getLocalPath());
            if (!objectFile.isFile() || objectFile.size() != asset.size) {
                missingAssets = true;
                break;
            }
        }
    }

    if (!missingAssets) {
        emitSucceeded();
        return;
    }

    setStatus(tr("Downloading missing game resources…"));
    m_assetTask = makeShared<AssetUpdateTask>(m_instance);
    connect(m_assetTask.get(), &Task::succeeded, this, [this] { emitSucceeded(); });
    connect(m_assetTask.get(), &Task::failed, this, [this](const QString& reason) { emitFailed(reason); });
    connect(m_assetTask.get(), &Task::aborted, this, [this] { emitAborted(); });
    connect(m_assetTask.get(), &Task::progress, this, &Task::setProgress);
    connect(m_assetTask.get(), &Task::status, this, &Task::setStatus);
    connect(m_assetTask.get(), &Task::details, this, &Task::setDetails);
    m_assetTask->start();
    emit progressReportingRequest();
}

bool EnsureOfflineAssets::abort()
{
    return m_assetTask ? m_assetTask->abort() : false;
}
