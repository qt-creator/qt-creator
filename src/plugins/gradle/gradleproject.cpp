// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradleproject.h"

#include <projectexplorer/projectmanager.h>

#include <QJsonDocument>
#include <QTextEdit>
#include <QVBoxLayout>

const QLatin1StringView GRADLE_MIMETYPE{"text/x-gradle"};

using namespace Utils;
using namespace ProjectExplorer;

namespace Gradle::Internal {

static QJsonObject defaultGradleJson()
{
    static QJsonObject result = QJsonDocument::fromJson(R"(
    {
        "build.configuration": [
            {
                "name": "Project Gradlew build",
                    "steps": [
                        {
                            "executable": "%{ActiveProject:ProjectDirectory}/gradlew%{HostOs:BatchFileSuffix}",
                            "workingDirectory": "%{ActiveProject:ProjectDirectory}"
                        }
                    ]
            },
            {
                "name": "System Gradle build",
                    "steps": [
                        {
                            "executable": "gradle",
                            "workingDirectory": "%{ActiveProject:ProjectDirectory}"
                        }
                    ]
            }
        ],
        "targets": [
            {
                "name": "Project Gradlew run",
                "executable": "%{ActiveProject:ProjectDirectory}/gradlew%{HostOs:BatchFileSuffix}",
                "arguments": ["run"],
                "workingDirectory": "%{ActiveProject:ProjectDirectory}"
            },
            {
                "name": "System Gradle run",
                "executable": "gradle",
                "arguments": ["run"],
                "workingDirectory": "%{ActiveProject:ProjectDirectory}"
            }
        ]
    })").object();
    return result;
}

class GradleProject : public WorkspaceProject
{
public:
    GradleProject(const FilePath &file)
        : WorkspaceProject(file.parentDir(), defaultGradleJson())
    {
    }
};

void setupGradleProject()
{
    ProjectManager::registerProjectType<GradleProject>(GRADLE_MIMETYPE);
}

} // namespace Gradle::Internal
