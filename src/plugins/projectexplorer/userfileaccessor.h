// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>
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
    QVariantMap postprocessMerge(const QVariantMap &main,
                                 const QVariantMap &secondary,
                                 const QVariantMap &result) const final;

    QVariantMap preprocessReadSettings(const QVariantMap &data) const final;
    QVariantMap prepareToWriteSettings(const QVariantMap &data) const final;

    Utils::SettingsMergeResult merge(const SettingsMergeData &global,
                                     const SettingsMergeData &local) const final;
private:
    Utils::SettingsMergeFunction userStickyTrackerFunction(QStringList &stickyKeys) const;

    Project *m_project;
};

} // namespace Internal
} // namespace ProjectExplorer
