// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cargoproject.h"

#include <projectexplorer/projectmanager.h>

#include <QTextEdit>
#include <QVBoxLayout>

const QLatin1StringView CARGO_TOML_MIMETYPE{"text/x-cargo-toml"};

using namespace Utils;
using namespace ProjectExplorer;

namespace Cargo::Internal {

static QJsonObject defaultCargoJson()
{
    static QJsonObject result = QJsonDocument::fromJson(R"(
    {
        "build.configuration": [
            {
                "name": "cargo build",
                    "steps": [
                        {
                            "executable": "cargo",
                            "arguments": ["build"],
                            "workingDirectory": "%{ActiveProject:ProjectDirectory}"
                        }
                    ]
            }
        ],
        "targets": [
            {
                "name": "cargo run",
                "executable": "cargo",
                "arguments": ["run"],
                "workingDirectory": "%{ActiveProject:ProjectDirectory}"
            }
        ]
    })").object();
    return result;
}

class CargoProject : public WorkspaceProject
{
public:
    CargoProject(const FilePath &file)
        : WorkspaceProject(file.parentDir(), defaultCargoJson())
    {
    }
};

void setupCargoProject()
{
    ProjectManager::registerProjectType<CargoProject>(CARGO_TOML_MIMETYPE);
}

} // namespace Cargo::Internal
