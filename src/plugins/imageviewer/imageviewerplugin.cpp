/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov.
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "imageviewerplugin.h"
#include "imageviewer.h"
#include "imageviewerfactory.h"
#include "imageviewerconstants.h"

#include <QAction>
#include <QCoreApplication>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/id.h>

namespace ImageViewer {
namespace Internal {

///////////////////////////////// ImageViewerPlugin //////////////////////////////////

bool ImageViewerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    (void) new ImageViewerFactory(this);

    return true;
}

static inline ImageViewer *currentImageViewer()
{
    return qobject_cast<ImageViewer *>(Core::EditorManager::currentEditor());
}

void ImageViewerPlugin::extensionsInitialized()
{
    QAction *a = registerNewAction(Core::Constants::ZOOM_IN);
    connect(a, &QAction::triggered, this, []() {
        if (ImageViewer *iv = currentImageViewer())
            iv->zoomIn();
    });

    a = registerNewAction(Core::Constants::ZOOM_OUT);
    connect(a, &QAction::triggered, this, []() {
        if (ImageViewer *iv = currentImageViewer())
            iv->zoomOut();
    });

    a = registerNewAction(Core::Constants::ZOOM_RESET);
    connect(a, &QAction::triggered, this, []() {
        if (ImageViewer *iv = currentImageViewer())
            iv->resetToOriginalSize();
    });

    a = registerNewAction(Constants::ACTION_FIT_TO_SCREEN, tr("Fit to Screen"),
                          QKeySequence(tr("Ctrl+=")));
    connect(a, &QAction::triggered, this, []() {
        if (ImageViewer *iv = currentImageViewer())
            iv->fitToScreen();
    });

    a = registerNewAction(Constants::ACTION_BACKGROUND, tr("Switch Background"),
                          QKeySequence(tr("Ctrl+[")));
    connect(a, &QAction::triggered, this, []() {
        if (ImageViewer *iv = currentImageViewer())
            iv->switchViewBackground();
    });

    a = registerNewAction(Constants::ACTION_OUTLINE, tr("Switch Outline"),
                          QKeySequence(tr("Ctrl+]")));
    connect(a, &QAction::triggered, this, []() {
        if (ImageViewer *iv = currentImageViewer())
            iv->switchViewOutline();
    });

    a = registerNewAction(Constants::ACTION_TOGGLE_ANIMATION, tr("Toggle Animation"),
                          QKeySequence());
    connect(a, &QAction::triggered, this, []() {
        if (ImageViewer *iv = currentImageViewer())
            iv->togglePlay();
    });

    a = registerNewAction(Constants::ACTION_EXPORT_IMAGE, tr("Export Image"),
                          QKeySequence());
    connect(a, &QAction::triggered, this, []() {
        if (ImageViewer *iv = currentImageViewer())
            iv->exportImage();
    });

    a = registerNewAction(Constants::ACTION_EXPORT_MULTI_IMAGES, tr("Export Multiple Images"),
                          QKeySequence());
    connect(a, &QAction::triggered, this, []() {
        if (ImageViewer *iv = currentImageViewer())
            iv->exportMultiImages();
    });

}

QAction *ImageViewerPlugin::registerNewAction(Core::Id id,
                                              const QString &title, const QKeySequence &key)
{
    Core::Context context(Constants::IMAGEVIEWER_ID);
    QAction *action = new QAction(title, this);
    Core::Command *command = Core::ActionManager::registerAction(action, id, context);
    if (!key.isEmpty())
        command->setDefaultKeySequence(key);
    return action;
}

} // namespace Internal
} // namespace ImageViewer
