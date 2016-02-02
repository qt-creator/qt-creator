/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "deployablefile.h"
#include "projectexplorer_export.h"

#include <utils/algorithm.h>

#include <QList>
#include <QSet>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT DeploymentData
{
public:
    void setFileList(const QList<DeployableFile> &files) { m_files = files; }

    void addFile(const DeployableFile &file)
    {
        for (int i = 0; i < m_files.size(); ++i) {
            if (m_files.at(i).localFilePath() == file.localFilePath()) {
                m_files[i] = file;
                return;
            }
        }
        m_files << file;
    }

    void addFile(const QString &localFilePath, const QString &remoteDirectory,
                 DeployableFile::Type type = DeployableFile::TypeNormal)
    {
        addFile(DeployableFile(localFilePath, remoteDirectory, type));
    }

    int fileCount() const { return m_files.count(); }
    DeployableFile fileAt(int index) const { return m_files.at(index); }
    QList<DeployableFile> allFiles() const { return m_files; }

    DeployableFile deployableForLocalFile(const QString &localFilePath) const
    {
        return Utils::findOrDefault(m_files, [&localFilePath](const DeployableFile &d) {
                                                return d.localFilePath().toString() == localFilePath;
                                             });
    }

    bool operator==(const DeploymentData &other) const
    {
        return m_files.toSet() == other.m_files.toSet();
    }

private:
    QList<DeployableFile> m_files;
};

inline bool operator!=(const DeploymentData &d1, const DeploymentData &d2)
{
    return !(d1 == d2);
}

} // namespace ProjectExplorer
