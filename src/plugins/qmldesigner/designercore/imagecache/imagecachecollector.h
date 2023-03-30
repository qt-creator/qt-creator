// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecachecollectorinterface.h"

#include <QPointer>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class Model;
class NotIndentingTextEditModifier;
class ImageCacheConnectionManager;
class RewriterView;
class NodeInstanceView;
class ExternalDependenciesInterface;

enum class ImageCacheCollectorNullImageHandling { CaptureNullImage, DontCaptureNullImage };

class ImageCacheCollector final : public ImageCacheCollectorInterface
{
public:
    ImageCacheCollector(ImageCacheConnectionManager &connectionManager,
                        QSize captureImageMinimumSize,
                        QSize captureImageMaximumSize,
                        ExternalDependenciesInterface &externalDependencies,
                        ImageCacheCollectorNullImageHandling nullImageHandling = {});

    ~ImageCacheCollector();

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
    ImageCacheConnectionManager &m_connectionManager;
    QPointer<ProjectExplorer::Target> m_target;
    QSize captureImageMinimumSize;
    QSize captureImageMaximumSize;
    ExternalDependenciesInterface &m_externalDependencies;
    ImageCacheCollectorNullImageHandling nullImageHandling{};
};

} // namespace QmlDesigner
