// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "antproject.h"

#include <projectexplorer/projectmanager.h>

#include <QJsonDocument>
#include <QTextEdit>
#include <QVBoxLayout>

const QLatin1StringView ANT_BUILDFILE_MIMETYPE{"text/x-ant-build"};

using namespace Utils;
using namespace ProjectExplorer;

namespace Ant::Internal {

static QJsonObject defaultAntJson()
{
    static QJsonObject result = QJsonDocument::fromJson(R"(
    {
        "build.configuration": [
            {
                "name": "ant build",
                    "steps": [
                        {
                            "executable": "ant",
                            "workingDirectory": "%{ActiveProject:ProjectDirectory}"
                        }
                    ]
            }
        ]
    })").object();
    return result;
}

class AntProject : public WorkspaceProject
{
public:
    AntProject(const FilePath &file)
        : WorkspaceProject(file.parentDir(), defaultAntJson())
    {
    }
};

void setupAntProject()
{
    ProjectManager::registerProjectType<AntProject>(ANT_BUILDFILE_MIMETYPE);
}

} // namespace Ant::Internal
