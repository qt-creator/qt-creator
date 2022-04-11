/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

enum class ImageCacheCollectorNullImageHandling { CaptureNullImage, DontCaptureNullImage };

class ImageCacheCollector final : public ImageCacheCollectorInterface
{
public:
    ImageCacheCollector(ImageCacheConnectionManager &connectionManager,
                        QSize captureImageMinimumSize,
                        QSize captureImageMaximumSize,
                        ImageCacheCollectorNullImageHandling nullImageHandling = {});

    ~ImageCacheCollector();

    void start(Utils::SmallStringView filePath,
               Utils::SmallStringView state,
               const ImageCache::AuxiliaryData &auxiliaryData,
               CaptureCallback captureCallback,
               AbortCallback abortCallback) override;

    std::pair<QImage, QImage> createImage(Utils::SmallStringView filePath,
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
    ImageCacheCollectorNullImageHandling nullImageHandling{};
};

} // namespace QmlDesigner
