// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourcegeneratorproxy.h"

#include <QStandardPaths>
#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <QtConcurrent>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <coreplugin/messagemanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/qtcprocess.h>

#include <qmldesigner/components/componentcore/resourcegenerator.h>

namespace QmlDesigner::DesignViewer {

ResourceGeneratorProxy::~ResourceGeneratorProxy()
{
    if (m_future.isRunning()) {
        m_future.cancel();
        m_future.waitForFinished();
    }
}

void ResourceGeneratorProxy::createResourceFileAsync(const QString &projectName)
{
    m_future = QtConcurrent::run([&]() {
        const std::optional<Utils::FilePath> filePath = createResourceFileSync(projectName);

        if (!filePath || filePath->isEmpty()) {
            emit errorOccurred("Failed to create resource file");
            return;
        }

        emit resourceFileCreated(filePath.value());
    });
}

std::optional<Utils::FilePath> ResourceGeneratorProxy::createResourceFileSync(const QString &projectName)
{
    const auto project = ProjectExplorer::ProjectManager::startupProject();
    std::optional<Utils::FilePath> resourcePath = project->projectDirectory().pathAppended(
        projectName + ".qmlrc");

    const bool retVal = ResourceGenerator::createQmlrcFile(resourcePath.value());

    if (!retVal || resourcePath->fileSize() == 0) {
        Core::MessageManager::writeDisrupting(tr("Failed to create resource file"));
        resourcePath.reset();
    }

    return resourcePath;
}

} // namespace QmlDesigner::DesignViewer
