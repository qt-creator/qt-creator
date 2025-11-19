// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dotnetproject.h"

#include <projectexplorer/projectmanager.h>

#include <QJsonDocument>
#include <QTextEdit>
#include <QVBoxLayout>

const QLatin1StringView CSPROJ_MIMETYPE{"text/x-csproj"};

using namespace Utils;
using namespace ProjectExplorer;

namespace Dotnet::Internal {

static QJsonObject defaultDotnetJson()
{
    static QJsonObject result = QJsonDocument::fromJson(R"(
    {
        "build.configuration": [
            {
                "name": "dotnet build",
                    "steps": [
                        {
                            "executable": "dotnet",
                            "arguments": ["build"],
                            "workingDirectory": "%{ActiveProject:ProjectDirectory}"
                        }
                    ]
            }
        ],
        "files.exclude": [
            "bin"
        ],
        "targets": [
            {
                "name": "dotnet run",
                "executable": "dotnet",
                "arguments": ["run"],
                "workingDirectory": "%{ActiveProject:ProjectDirectory}"
            }
        ]
    })").object();
    return result;
}

class DotnetProject : public WorkspaceProject
{
public:
    DotnetProject(const FilePath &file)
        : WorkspaceProject(file.parentDir(), defaultDotnetJson())
    {
    }
};

void setupDotnetProject()
{
    ProjectManager::registerProjectType<DotnetProject>(CSPROJ_MIMETYPE);
}

} // namespace Dotnet::Internal
