
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "cmakegenerator.h"
#include "pythongenerator.h"

namespace QmlProjectManager {
namespace QmlProjectExporter {

class Exporter : public QObject
{
    Q_OBJECT

public:
    Exporter(QmlBuildSystem *bs = nullptr);
    virtual ~Exporter() = default;

    void updateMenuAction();
    void updateProject(QmlProject *project);
    void updateProjectItem(QmlProjectItem *item, bool updateEnabled);

private:
    CMakeGenerator *m_cmakeGen;
    PythonGenerator *m_pythonGen;
};

} // namespace QmlProjectExporter.
} // namespace QmlProjectManager.
