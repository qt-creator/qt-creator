// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    return new SubDirFileIterator(FilePaths(dirs.constBegin(), dirs.constEnd()),
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
