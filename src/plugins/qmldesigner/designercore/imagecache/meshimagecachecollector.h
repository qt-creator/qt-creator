// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecachecollectorinterface.h"
#include "imagecachecollector.h"

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class ImageCacheConnectionManager;

class MeshImageCacheCollector final : public ImageCacheCollectorInterface
{
public:
    MeshImageCacheCollector(ImageCacheConnectionManager &connectionManager,
                            QSize captureImageMinimumSize,
                            QSize captureImageMaximumSize,
                            ExternalDependenciesInterface &externalDependencies,
                            ImageCacheCollectorNullImageHandling nullImageHandling = {});

    ~MeshImageCacheCollector();

    void start(Utils::SmallStringView filePath,
               Utils::SmallStringView state,
               const ImageCache::AuxiliaryData &auxiliaryData,
               CaptureCallback captureCallback,
               AbortCallback abortCallback) override;

    ImageTuple createImage(Utils::SmallStringView filePath,
                           Utils::SmallStringView state,
                           const ImageCache::AuxiliaryData &auxiliaryData) override;

    QIcon createIcon(Utils::SmallStringView filePath,
                     Utils::SmallStringView state,
                     const ImageCache::AuxiliaryData &auxiliaryData) override;

    void setTarget(ProjectExplorer::Target *target);
    ProjectExplorer::Target *target() const;

private:
    ImageCacheCollector m_imageCacheCollector;
};

} // namespace QmlDesigner
