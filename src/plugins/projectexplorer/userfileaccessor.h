/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
