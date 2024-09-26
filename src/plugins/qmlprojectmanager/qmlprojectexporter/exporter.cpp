// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "exporter.h"

#include "../buildsystem/qmlbuildsystem.h"
#include "../buildsystem/projectitem/qmlprojectitem.h"

namespace QmlProjectManager {
namespace QmlProjectExporter {

Exporter::Exporter(QmlBuildSystem *bs)
    : QObject(bs)
    , m_cmakeGen(new CMakeGenerator(bs))
    , m_pythonGen(new PythonGenerator(bs))
{}

void Exporter::updateMenuAction()
{
    m_cmakeGen->updateMenuAction();
    m_pythonGen->updateMenuAction();
}

void Exporter::updateProject(QmlProject *project)
{
    m_cmakeGen->updateProject(project);
    m_pythonGen->updateProject(project);
}

void Exporter::updateProjectItem(QmlProjectItem *item, bool updateEnabled)
{
    connect(item, &QmlProjectItem::filesChanged, m_cmakeGen, &CMakeGenerator::update);

    if (updateEnabled) {
        m_cmakeGen->setEnabled(item->enableCMakeGeneration());
        m_pythonGen->setEnabled(item->enablePythonGeneration());
    }
}

} // namespace QmlProjectExporter.
} // namespace QmlProjectManager.
