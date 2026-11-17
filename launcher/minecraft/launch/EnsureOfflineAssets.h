// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "launch/LaunchStep.h"
#include "minecraft/MinecraftInstance.h"
#include "tasks/Task.h"

class EnsureOfflineAssets final : public LaunchStep {
    Q_OBJECT

   public:
    explicit EnsureOfflineAssets(LaunchTask* parent, MinecraftInstance* instance);
    ~EnsureOfflineAssets() override = default;

    void executeTask() override;
    bool canAbort() const override { return m_assetTask && m_assetTask->canAbort(); }
    bool abort() override;

   private:
    MinecraftInstance* m_instance;
    Task::Ptr m_assetTask;
};
