// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/settingsaccessor.h>

#include <QHash>
#include <QVariantMap>
#include <QMessageBox>

namespace ProjectExplorer {

class Project;

namespace Internal {

class UserFileAccessor : public Utils::MergingSettingsAccessor
{
public:
    UserFileAccessor(Project *project);

    Project *project() const;

    virtual QVariant retrieveSharedSettings() const;

    Utils::FilePath projectUserFile() const;
    Utils::FilePath externalUserFile() const;
    Utils::FilePath sharedFile() const;

protected:
    Utils::Store postprocessMerge(const Utils::Store &main,
                                  const Utils::Store &secondary,
                                  const Utils::Store &result) const final;

    Utils::Store preprocessReadSettings(const Utils::Store &data) const final;
    Utils::Store prepareToWriteSettings(const Utils::Store &data) const final;

    Utils::SettingsMergeResult merge(const SettingsMergeData &global,
                                     const SettingsMergeData &local) const final;
private:
    Utils::SettingsMergeFunction userStickyTrackerFunction(Utils::KeyList &stickyKeys) const;

    Project *m_project;
};

} // namespace Internal
} // namespace ProjectExplorer
