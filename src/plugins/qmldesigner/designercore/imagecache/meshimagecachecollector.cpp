// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "meshimagecachecollector.h"
#include "imagecacheconnectionmanager.h"

#include <projectexplorer/target.h>
#include <utils/smallstring.h>
#include <qtsupport/qtkitaspect.h>

#include <QTemporaryFile>

namespace QmlDesigner {

MeshImageCacheCollector::MeshImageCacheCollector(ImageCacheConnectionManager &connectionManager,
                                                 QSize captureImageMinimumSize,
                                                 QSize captureImageMaximumSize,
                                                 ExternalDependenciesInterface &externalDependencies,
                                                 ImageCacheCollectorNullImageHandling nullImageHandling)
    : m_imageCacheCollector(connectionManager,
                            captureImageMinimumSize,
                            captureImageMaximumSize,
                            externalDependencies,
                            nullImageHandling)
{}

MeshImageCacheCollector::~MeshImageCacheCollector() = default;

void MeshImageCacheCollector::start(Utils::SmallStringView name,
                                    Utils::SmallStringView state,
                                    const ImageCache::AuxiliaryData &auxiliaryData,
                                    CaptureCallback captureCallback,
                                    AbortCallback abortCallback)
{
    QTemporaryFile file(QDir::tempPath() + "/mesh-XXXXXX.qml");
    if (file.open()) {
        QString qtQuickVersion;
        QString qtQuick3DVersion;
        if (target()) {
            QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(target()->kit());
            if (qtVersion && qtVersion->qtVersion() < QVersionNumber(6, 0, 0)) {
                qtQuickVersion = "2.15";
                qtQuick3DVersion = "1.15";
            }
        }

        QString content{
            R"(import QtQuick %1
               import QtQuick3D %2
               Node {
                   Model {
                       source: "%3"
                       DefaultMaterial { id: defaultMaterial; diffuseColor: "#ff999999" }
                       materials: [ defaultMaterial ]
                   }
               })"};

        content = content.arg(qtQuickVersion, qtQuick3DVersion, QString(name));

        file.write(content.toUtf8());
        file.close();
    }

    Utils::PathString path{file.fileName()};

    m_imageCacheCollector.start(path, state, auxiliaryData, captureCallback, abortCallback);
}

ImageCacheCollectorInterface::ImageTuple MeshImageCacheCollector::createImage(
    Utils::SmallStringView, Utils::SmallStringView, const ImageCache::AuxiliaryData &)
{
    return {};
}

QIcon MeshImageCacheCollector::createIcon(Utils::SmallStringView,
                                          Utils::SmallStringView,
                                          const ImageCache::AuxiliaryData &)
{
    return {};
}

void MeshImageCacheCollector::setTarget(ProjectExplorer::Target *target)
{
    m_imageCacheCollector.setTarget(target);
}

ProjectExplorer::Target *MeshImageCacheCollector::target() const
{
    return m_imageCacheCollector.target();
}

} // namespace QmlDesigner
