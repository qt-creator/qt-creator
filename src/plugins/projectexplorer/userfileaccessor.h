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

protected:
    Utils::Store prepareToWriteSettings(const Utils::Store &data) const final;

private:
    Utils::Store postprocessMerge(const Utils::Store &main,
                                  const Utils::Store &secondary,
                                  const Utils::Store &result) const final;

    Utils::SettingsMergeResult merge(const SettingsMergeData &global,
                                     const SettingsMergeData &local) const final;
    std::optional<Issue> writeFile(const Utils::FilePath &path, const Utils::Store &data) const final;

    virtual QVariant retrieveSharedSettings() const;

    Utils::FilePath projectUserFileV1() const;
    Utils::FilePath projectUserFileV2() const;
    Utils::FilePath externalUserFile() const;
    Utils::FilePath sharedFile() const;
    Utils::SettingsMergeFunction userStickyTrackerFunction(Utils::KeyList &stickyKeys) const;

    Project * const m_project;
};

QObject *createUserFileAccessorTest();

} // namespace Internal
} // namespace ProjectExplorer
