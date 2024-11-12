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

void ResourceGeneratorProxy::createResourceFileAsync()
{
    m_future = QtConcurrent::run([&]() {
        const QString filePath = createResourceFileSync();

        if (filePath.isEmpty()) {
            emit errorOccurred("Failed to create resource file");
            return;
        }

        emit resourceFileCreated(filePath);
    });
}

QString ResourceGeneratorProxy::createResourceFileSync(const QString &projectName)
{
    const auto project = ProjectExplorer::ProjectManager::startupProject();
    const Utils::FilePath tempFilePath = project->projectDirectory().pathAppended(projectName
                                                                                  + ".qmlrc");

    const bool retVal = ResourceGenerator::createQmlrcFile(tempFilePath);

    if (!retVal || tempFilePath.fileSize() == 0) {
        Core::MessageManager::writeDisrupting(tr("Failed to create resource file"));
        return "";
    }

    return tempFilePath.toString();
}

} // namespace QmlDesigner::DesignViewer
