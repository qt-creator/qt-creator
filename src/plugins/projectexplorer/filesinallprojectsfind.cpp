// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesinallprojectsfind.h"

#include "project.h"
#include "projectexplorertr.h"
#include "projectmanager.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/algorithm.h>
#include <utils/qtcsettings.h>

using namespace TextEditor;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

QString FilesInAllProjectsFind::id() const
{
    return QLatin1String("Files in All Project Directories");
}

QString FilesInAllProjectsFind::displayName() const
{
    return Tr::tr("Files in All Project Directories");
}

const char kSettingsKey[] = "FilesInAllProjectDirectories";

void FilesInAllProjectsFind::writeSettings(QtcSettings *settings)
{
    settings->beginGroup(kSettingsKey);
    writeCommonSettings(settings);
    settings->endGroup();
}

void FilesInAllProjectsFind::readSettings(QtcSettings *settings)
{
    settings->beginGroup(kSettingsKey);
    readCommonSettings(
        settings,
        "CMakeLists.txt,*.cmake,*.pro,*.pri,*.qbs,*.cpp,*.h,*.mm,*.qml,*.md,*.txt,*.qdoc",
        "*/.git/*,*/.cvs/*,*/.svn/*,*.autosave");
    settings->endGroup();
}

FileContainerProvider FilesInAllProjectsFind::fileContainerProvider() const
{
    return [nameFilters = fileNameFilters(), exclusionFilters = fileExclusionFilters()] {
        const QSet<FilePath> dirs = Utils::transform<QSet>(ProjectManager::projects(), [](Project *p) {
            return p->projectFilePath().parentDir();
        });
        return SubDirFileContainer(FilePaths(dirs.constBegin(), dirs.constEnd()), nameFilters,
                                   exclusionFilters, Core::EditorManager::defaultTextCodec());
    };
}

QString FilesInAllProjectsFind::label() const
{
    return Tr::tr("Files in All Project Directories:");
}

} // namespace Internal
} // namespace ProjectExplorer
