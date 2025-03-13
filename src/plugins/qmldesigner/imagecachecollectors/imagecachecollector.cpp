// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imagecachecollector.h"

#include <externaldependenciesinterface.h>
#include <qmlpuppetpaths.h>
#include <qprocessuniqueptr.h>

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QGuiApplication>
#include <QProcess>
#include <QTemporaryDir>

namespace QmlDesigner {

namespace {

} // namespace

ImageCacheCollector::ImageCacheCollector(QSize captureImageMinimumSize,
                                         QSize captureImageMaximumSize,
                                         ExternalDependenciesInterface &externalDependencies,
                                         ImageCacheCollectorNullImageHandling nullImageHandling)
    : captureImageMinimumSize{captureImageMinimumSize}
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
                                Utils::SmallStringView,
                                const ImageCache::AuxiliaryData &auxiliaryData,
                                CaptureCallback captureCallback,
                                AbortCallback abortCallback,
                                ImageCache::TraceToken traceToken)
{
    using namespace NanotraceHR::Literals;
    auto [collectorTraceToken, flowtoken] = traceToken.beginDurationWithFlow(
        "generate image in standard collector");

    QTemporaryDir outDir(QDir::tempPath() + "/qds_imagecache_XXXXXX");
    QString outFile = outDir.filePath("capture.png");

    QImage captureImage;
    if (runProcess(createArguments(name, outFile, auxiliaryData))) {
        captureImage.load(outFile);
    } else {
        if (abortCallback)
            abortCallback(ImageCache::AbortReason::Failed, std::move(flowtoken));
        return;
    }

    if (nullImageHandling == ImageCacheCollectorNullImageHandling::CaptureNullImage
        || !captureImage.isNull()) {
        QImage midSizeImage = scaleImage(captureImage, QSize{300, 300});
        QImage smallImage = scaleImage(midSizeImage, QSize{96, 96});
        captureCallback(captureImage, midSizeImage, smallImage, std::move(flowtoken));
    } else {
        if (abortCallback)
            abortCallback(ImageCache::AbortReason::Failed, std::move(flowtoken));
        return;
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

QStringList ImageCacheCollector::createArguments(Utils::SmallStringView name,
                                                 const QString &outFile,
                                                 const ImageCache::AuxiliaryData &auxiliaryData) const
{
    QStringList arguments;
    const QString filePath{name};

    arguments.append("--qml-renderer");
    arguments.append(filePath);

    if (m_target && m_target->project()) {
        arguments.append("-i");
        arguments.append(m_target->project()->projectDirectory().toFSPathString());
    }

    arguments.append("-o");
    arguments.append(outFile);

    if (std::holds_alternative<ImageCache::LibraryIconAuxiliaryData>(auxiliaryData))
        arguments.append("--libIcon");

    if (captureImageMinimumSize.isValid()) {
        arguments.append("--minW");
        arguments.append(QString::number(captureImageMinimumSize.width()));
        arguments.append("--minH");
        arguments.append(QString::number(captureImageMinimumSize.height()));
    }

    if (captureImageMaximumSize.isValid()) {
        arguments.append("--maxW");
        arguments.append(QString::number(captureImageMaximumSize.width()));
        arguments.append("--maxH");
        arguments.append(QString::number(captureImageMaximumSize.height()));
    }

    return arguments;
}

bool ImageCacheCollector::runProcess(const QStringList &arguments) const
{
    if (!m_target)
        return false;

    auto [workingDirectoryPath, puppetPath] = QmlDesigner::QmlPuppetPaths::qmlPuppetPaths(
        target()->kit(), m_externalDependencies.designerSettings());
    if (puppetPath.isEmpty())
        return false;

    QProcessUniquePointer puppetProcess{new QProcess};

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("QSG_RHI_BACKEND");
    puppetProcess->setProcessEnvironment(env);

    QObject::connect(QCoreApplication::instance(),
                     &QCoreApplication::aboutToQuit,
                     puppetProcess.get(),
                     &QProcess::kill);

    puppetProcess->setWorkingDirectory(workingDirectoryPath.toFSPathString());
    puppetProcess->setProcessChannelMode(QProcess::ForwardedChannels);

    puppetProcess->start(puppetPath.toFSPathString(), arguments);

    if (puppetProcess->waitForFinished(30000)) {
        return puppetProcess->exitStatus() == QProcess::ExitStatus::NormalExit
               && puppetProcess->exitCode() == 0;
    }

    return false;
}

} // namespace QmlDesigner
