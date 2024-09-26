// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imagecachecollector.h"
#include "imagecacheconnectionmanager.h"

#include <model.h>
#include <nodeinstanceview.h>
#include <nodemetainfo.h>
#include <plaintexteditmodifier.h>
#include <rewriterview.h>

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <utils/fileutils.h>

#include <QGuiApplication>
#include <QPlainTextEdit>

namespace QmlDesigner {

namespace {

QByteArray fileToByteArray(const QString &filename)
{
    QFile file(filename);
    QFileInfo fleInfo(file);

    if (fleInfo.exists() && file.open(QFile::ReadOnly))
        return file.readAll();

    return {};
}

QString fileToString(const QString &filename)
{
    return QString::fromUtf8(fileToByteArray(filename));
}

} // namespace

ImageCacheCollector::ImageCacheCollector(ImageCacheConnectionManager &connectionManager,
                                         QSize captureImageMinimumSize,
                                         QSize captureImageMaximumSize,
                                         ExternalDependenciesInterface &externalDependencies,
                                         ImageCacheCollectorNullImageHandling nullImageHandling)
    : m_connectionManager{connectionManager}
    , captureImageMinimumSize{captureImageMinimumSize}
    , captureImageMaximumSize{captureImageMaximumSize}
    , m_externalDependencies{externalDependencies}
    , nullImageHandling{nullImageHandling}
{}

ImageCacheCollector::~ImageCacheCollector() = default;

namespace {
QImage scaleImage(const QImage &image, QSize targetSize)
{
    if (image.isNull())
        return {};

    const qreal ratio = qGuiApp->devicePixelRatio();
    if (ratio > 1.0)
        targetSize *= qRound(ratio);
    QSize scaledImageSize = image.size().scaled(targetSize.boundedTo(image.size()),
                                                Qt::KeepAspectRatio);
    return image.scaled(scaledImageSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

}
} // namespace

void ImageCacheCollector::start(Utils::SmallStringView name,
                                Utils::SmallStringView state,
                                const ImageCache::AuxiliaryData &auxiliaryData,
                                CaptureCallback captureCallback,
                                AbortCallback abortCallback,
                                ImageCache::TraceToken traceToken)
{
#ifdef QDS_USE_PROJECTSTORAGE
    if (!m_projectStorage || !m_pathCache)
        return;
#endif

    using namespace NanotraceHR::Literals;
    auto [collectorTraceToken, flowtoken] = traceToken.beginDurationWithFlow(
        "generate image in standard collector"_t);

    RewriterView rewriterView{m_externalDependencies, RewriterView::Amend};
    NodeInstanceView nodeInstanceView{m_connectionManager, m_externalDependencies};
    nodeInstanceView.setCaptureImageMinimumAndMaximumSize(captureImageMinimumSize,
                                                          captureImageMaximumSize);

    const QString filePath{name};
#ifdef QDS_USE_PROJECTSTORAGE
    auto model = QmlDesigner::Model::create({*m_projectStorage, *m_pathCache},
                                            "Item",
                                            {},
                                            QUrl::fromLocalFile(filePath));
#else
    auto model = QmlDesigner::Model::create("QtQuick/Item", 2, 1);
    model->setFileUrl(QUrl::fromLocalFile(filePath));
#endif

    auto textDocument = std::make_unique<QTextDocument>(fileToString(filePath));

    auto modifier = std::make_unique<NotIndentingTextEditModifier>(textDocument.get(),
                                                                   QTextCursor{textDocument.get()});

    rewriterView.setTextModifier(modifier.get());

    model->setRewriterView(&rewriterView);

    auto rootModelNodeMetaInfo = rewriterView.rootModelNode().metaInfo();
    bool is3DRoot = rewriterView.errors().isEmpty()
                    && (rootModelNodeMetaInfo.isQtQuick3DNode()
                        || rootModelNodeMetaInfo.isQtQuick3DMaterial());

    if (!rewriterView.errors().isEmpty() || (!rewriterView.rootModelNode().metaInfo().isGraphicalItem()
                                        && !is3DRoot)) {
        if (abortCallback)
            abortCallback(ImageCache::AbortReason::Failed, std::move(flowtoken));
        return;
    }

    if (is3DRoot) {
        if (auto libIcon = std::get_if<ImageCache::LibraryIconAuxiliaryData>(&auxiliaryData))
            rewriterView.rootModelNode().setAuxiliaryData(AuxiliaryDataType::NodeInstancePropertyOverwrite,
                                                          "isLibraryIcon",
                                                          libIcon->enable);
    }

    ModelNode stateNode = rewriterView.modelNodeForId(QString{state});

    if (stateNode.isValid())
        rewriterView.setCurrentStateNode(stateNode);

    QImage captureImage;

    auto callback = [&](const QImage &image) { captureImage = image; };

    if (!m_target)
        return;

    nodeInstanceView.setTarget(m_target.data());
    m_connectionManager.setCallback(std::move(callback));
    bool isCrashed = false;
    nodeInstanceView.setCrashCallback([&] { isCrashed = true; });
    model->setNodeInstanceView(&nodeInstanceView);

    bool capturedDataArrived = m_connectionManager.waitForCapturedData();

    m_connectionManager.setCallback({});
    m_connectionManager.setCrashCallback({});

    model->setNodeInstanceView({});
    model->setRewriterView({});

    if (isCrashed)
        abortCallback(ImageCache::AbortReason::Failed, std::move(flowtoken));

    if (!capturedDataArrived && abortCallback)
        abortCallback(ImageCache::AbortReason::Failed, std::move(flowtoken));

    if (nullImageHandling == ImageCacheCollectorNullImageHandling::CaptureNullImage
        || !captureImage.isNull()) {
        QImage midSizeImage = scaleImage(captureImage, QSize{300, 300});
        QImage smallImage = scaleImage(midSizeImage, QSize{96, 96});
        captureCallback(captureImage, midSizeImage, smallImage, std::move(flowtoken));
    }
}

ImageCacheCollectorInterface::ImageTuple ImageCacheCollector::createImage(
    Utils::SmallStringView, Utils::SmallStringView, const ImageCache::AuxiliaryData &)
{
    return {};
}

QIcon ImageCacheCollector::createIcon(Utils::SmallStringView,
                                      Utils::SmallStringView,
                                      const ImageCache::AuxiliaryData &)
{
    return {};
}

void ImageCacheCollector::setTarget(ProjectExplorer::Target *target)
{
    m_target = target;
}

ProjectExplorer::Target *ImageCacheCollector::target() const
{
    return m_target;
}

} // namespace QmlDesigner
