// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <imagecache/imagecachecollectorinterface.h>

#include <QPointer>

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class ExternalDependenciesInterface;

enum class ImageCacheCollectorNullImageHandling { CaptureNullImage, DontCaptureNullImage };

class ImageCacheCollector final : public ImageCacheCollectorInterface
{
public:
    ImageCacheCollector(QSize captureImageMinimumSize,
                        QSize captureImageMaximumSize,
                        ExternalDependenciesInterface &externalDependencies,
                        ImageCacheCollectorNullImageHandling nullImageHandling = {});

    ~ImageCacheCollector();

    void start(Utils::SmallStringView filePath,
               Utils::SmallStringView state,
               const ImageCache::AuxiliaryData &auxiliaryData,
               CaptureCallback captureCallback,
               AbortCallback abortCallback,
               ImageCache::TraceToken traceToken) override;

    ImageTuple createImage(Utils::SmallStringView filePath,
                           Utils::SmallStringView state,
                           const ImageCache::AuxiliaryData &auxiliaryData) override;

    QIcon createIcon(Utils::SmallStringView filePath,
                     Utils::SmallStringView state,
                     const ImageCache::AuxiliaryData &auxiliaryData) override;

    void setTarget(ProjectExplorer::Target *target);
    ProjectExplorer::Target *target() const;

private:
    bool runProcess(const QStringList &arguments) const;
    QStringList createArguments(Utils::SmallStringView name,
                                const QString &outFile,
                                const ImageCache::AuxiliaryData &auxiliaryData) const;

private:
    QPointer<ProjectExplorer::Target> m_target;
    QSize captureImageMinimumSize;
    QSize captureImageMaximumSize;
    ExternalDependenciesInterface &m_externalDependencies;
    ImageCacheCollectorNullImageHandling nullImageHandling{};
};

} // namespace QmlDesigner
