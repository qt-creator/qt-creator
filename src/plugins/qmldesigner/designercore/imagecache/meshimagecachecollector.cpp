/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "meshimagecachecollector.h"
#include "imagecacheconnectionmanager.h"

#include <projectexplorer/target.h>
#include <utils/smallstring.h>
#include <qtsupport/qtkitinformation.h>

#include <QTemporaryFile>

namespace QmlDesigner {

MeshImageCacheCollector::MeshImageCacheCollector(
    ImageCacheConnectionManager &connectionManager,
    QSize captureImageMinimumSize,
    QSize captureImageMaximumSize,
    ImageCacheCollectorNullImageHandling nullImageHandling)
    : m_imageCacheCollector(connectionManager,
                            captureImageMinimumSize,
                            captureImageMaximumSize,
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
            if (qtVersion && qtVersion->qtVersion() < QtSupport::QtVersionNumber(6, 0, 0)) {
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

std::pair<QImage, QImage> MeshImageCacheCollector::createImage(Utils::SmallStringView,
                                                               Utils::SmallStringView,
                                                               const ImageCache::AuxiliaryData &)
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
