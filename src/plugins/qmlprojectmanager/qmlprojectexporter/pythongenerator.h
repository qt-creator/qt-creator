// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "filegenerator.h"
#include <utils/filepath.h>

namespace QmlProjectManager {

class QmlProject;
class QmlProjectItem;
class QmlBuildSystem;

namespace QmlProjectExporter {

class PythonGenerator : public FileGenerator
{
    Q_OBJECT

public:
    static void createMenuAction(QObject *parent);

    PythonGenerator(QmlBuildSystem *bs);
    void updateMenuAction() override;
    void updateProject(QmlProject *project) override;

private:
    QString settingsFileContent() const;
};

} // namespace QmlProjectExporter.
} // namespace QmlProjectManager.
