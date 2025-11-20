// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "swiftproject.h"

#include <projectexplorer/projectmanager.h>

#include <QJsonDocument>
#include <QTextEdit>
#include <QVBoxLayout>

const QLatin1StringView SWIFT_MANIFEST_MIMETYPE{"text/x-swift-manifest"};

using namespace Utils;
using namespace ProjectExplorer;

namespace Swift::Internal {

static QJsonObject defaultSwiftJson()
{
    static QJsonObject result = QJsonDocument::fromJson(R"(
    {
        "build.configuration": [
            {
                "name": "swift build",
                    "steps": [
                        {
                            "executable": "swift",
                            "arguments": ["build"],
                            "workingDirectory": "%{ActiveProject:ProjectDirectory}"
                        }
                    ]
            }
        ],
        "files.exclude": [
            ".build"
        ],
        "targets": [
            {
                "name": "swift run",
                "executable": "swift",
                "arguments": ["run"],
                "workingDirectory": "%{ActiveProject:ProjectDirectory}"
            }
        ]
    })").object();
    return result;
}

class SwiftProject : public WorkspaceProject
{
public:
    SwiftProject(const FilePath &file)
        : WorkspaceProject(file.parentDir(), defaultSwiftJson())
    {
    }
};

void setupSwiftProject()
{
    ProjectManager::registerProjectType<SwiftProject>(SWIFT_MANIFEST_MIMETYPE);
}

} // namespace Swift::Internal
