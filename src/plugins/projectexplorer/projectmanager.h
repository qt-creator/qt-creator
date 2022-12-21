// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <QString>

#include <functional>

namespace Utils {
class FilePath;
class MimeType;
} // Utils

namespace ProjectExplorer {

class Project;

class PROJECTEXPLORER_EXPORT ProjectManager
{
public:
    static bool canOpenProjectForMimeType(const Utils::MimeType &mt);
    static Project *openProject(const Utils::MimeType &mt, const Utils::FilePath &fileName);

    template <typename T>
    static void registerProjectType(const QString &mimeType)
    {
        ProjectManager::registerProjectCreator(mimeType, [](const Utils::FilePath &fileName) {
            return new T(fileName);
        });
    }

private:
    static void registerProjectCreator(const QString &mimeType,
                                       const std::function<Project *(const Utils::FilePath &)> &);
};

} // namespace ProjectExplorer
