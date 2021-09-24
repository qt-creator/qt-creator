/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "filesinallprojectsfind.h"

#include "project.h"
#include "session.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>
#include <utils/filesearch.h>

#include <QSettings>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

QString FilesInAllProjectsFind::id() const
{
    return QLatin1String("Files in All Project Directories");
}

QString FilesInAllProjectsFind::displayName() const
{
    return tr("Files in All Project Directories");
}

const char kSettingsKey[] = "FilesInAllProjectDirectories";

void FilesInAllProjectsFind::writeSettings(QSettings *settings)
{
    settings->beginGroup(kSettingsKey);
    writeCommonSettings(settings);
    settings->endGroup();
}

void FilesInAllProjectsFind::readSettings(QSettings *settings)
{
    settings->beginGroup(kSettingsKey);
    readCommonSettings(
        settings,
        "CMakeLists.txt,*.cmake,*.pro,*.pri,*.qbs,*.cpp,*.h,*.mm,*.qml,*.md,*.txt,*.qdoc",
        "*/.git/*,*/.cvs/*,*/.svn/*,*.autosave");
    settings->endGroup();
}

Utils::FileIterator *FilesInAllProjectsFind::files(const QStringList &nameFilters,
                                                   const QStringList &exclusionFilters,
                                                   const QVariant &additionalParameters) const
{
    Q_UNUSED(additionalParameters)
    const QSet<FilePath> dirs = Utils::transform<QSet>(SessionManager::projects(), [](Project *p) {
        return p->projectFilePath().parentDir();
    });
    const QStringList dirStrings = Utils::transform<QStringList>(dirs, &FilePath::toString);
    return new SubDirFileIterator(dirStrings,
                                  nameFilters,
                                  exclusionFilters,
                                  Core::EditorManager::defaultTextCodec());
}

QString FilesInAllProjectsFind::label() const
{
    return tr("Files in All Project Directories:");
}

} // namespace Internal
} // namespace ProjectExplorer
