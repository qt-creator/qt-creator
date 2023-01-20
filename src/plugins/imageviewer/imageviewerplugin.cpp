// Copyright (C) 2016 Denis Mingulov.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imageviewerplugin.h"

#include "imageviewer.h"
#include "imageviewerconstants.h"
#include "imageviewertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QAction>
#include <QCoreApplication>
#include <QKeySequence>

using namespace Core;
using namespace Utils;

namespace ImageViewer::Internal {

class ImageViewerAction final : public QAction
{
public:
    ImageViewerAction(Id id,
                      const std::function<void(ImageViewer *v)> &onTriggered,
                      const QString &title = {},
                      const QKeySequence &key = {})
        : QAction(title)
    {
        Command *command = ActionManager::registerAction(this, id, Context(Constants::IMAGEVIEWER_ID));
        if (!key.isEmpty())
            command->setDefaultKeySequence(key);

        connect(this, &QAction::triggered, this, [onTriggered] {
            if (auto iv = qobject_cast<ImageViewer *>(EditorManager::currentEditor()))
                onTriggered(iv);
        });
    }
};

class ImageViewerPluginPrivate final
{
public:
    ImageViewerFactory imageViewerFactory;

    ImageViewerAction zoomInAction {
        Core::Constants::ZOOM_IN,
        &ImageViewer::zoomIn
    };

    ImageViewerAction zoomOutAction {
        Core::Constants::ZOOM_OUT,
        &ImageViewer::zoomOut
    };

    ImageViewerAction zoomResetAction {
        Core::Constants::ZOOM_RESET,
        &ImageViewer::resetToOriginalSize
    };

    ImageViewerAction fitToScreenAction {
        Constants::ACTION_FIT_TO_SCREEN,
        &ImageViewer::fitToScreen,
        Tr::tr("Fit to Screen"),
        Tr::tr("Ctrl+=")
    };

    ImageViewerAction switchBackgroundAction {
        Constants::ACTION_BACKGROUND,
        &ImageViewer::switchViewBackground,
        Tr::tr("Switch Background"),
        Tr::tr("Ctrl+[")
    };

    ImageViewerAction switchOutlineAction {
        Constants::ACTION_OUTLINE,
        &ImageViewer::switchViewOutline,
        Tr::tr("Switch Outline"),
        Tr::tr("Ctrl+]")
    };

    ImageViewerAction toggleAnimationAction {
        Constants::ACTION_TOGGLE_ANIMATION,
        &ImageViewer::togglePlay,
        Tr::tr("Toggle Animation")
    };

    ImageViewerAction exportImageAction {
        Constants::ACTION_EXPORT_IMAGE,
        &ImageViewer::exportImage,
        Tr::tr("Export Image")
    };

    ImageViewerAction exportMulitImagesAction {
        Constants::ACTION_EXPORT_MULTI_IMAGES,
        &ImageViewer::exportMultiImages,
        Tr::tr("Export Multiple Images"),
    };

    ImageViewerAction copyDataUrlAction {
        Constants::ACTION_COPY_DATA_URL,
        &ImageViewer::copyDataUrl,
        Tr::tr("Copy as Data URL"),
    };
};

ImageViewerPlugin::~ImageViewerPlugin()
{
    delete  d;
}

void ImageViewerPlugin::initialize()
{
    d = new ImageViewerPluginPrivate;
}

} // ImageViewer::Internal
