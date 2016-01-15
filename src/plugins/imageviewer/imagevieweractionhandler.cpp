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

#include "imagevieweractionhandler.h"
#include "imageviewer.h"
#include "imageviewerconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/id.h>

#include <QAction>
#include <QList>
#include <QSignalMapper>

namespace ImageViewer {
namespace Internal {

enum SupportedActions {
    ZoomIn = 0,
    ZoomOut,
    OriginalSize,
    FitToScreen,
    Background,
    Outline,
    ToggleAnimation
};

ImageViewerActionHandler::ImageViewerActionHandler(QObject *parent) :
    QObject(parent), m_signalMapper(new QSignalMapper(this))
{
    connect(m_signalMapper, SIGNAL(mapped(int)), SLOT(actionTriggered(int)));
}

void ImageViewerActionHandler::actionTriggered(int supportedAction)
{
    Core::IEditor *editor = Core::EditorManager::currentEditor();
    ImageViewer *viewer = qobject_cast<ImageViewer *>(editor);
    if (!viewer)
        return;

    SupportedActions action = static_cast<SupportedActions>(supportedAction);
    switch (action) {
    case ZoomIn:
        viewer->zoomIn();
        break;
    case ZoomOut:
        viewer->zoomOut();
        break;
    case OriginalSize:
        viewer->resetToOriginalSize();
        break;
    case FitToScreen:
        viewer->fitToScreen();
        break;
    case Background:
        viewer->switchViewBackground();
        break;
    case Outline:
        viewer->switchViewOutline();
        break;
    case ToggleAnimation:
        viewer->togglePlay();
        break;
    default:
        break;
    }
}

void ImageViewerActionHandler::createActions()
{
    registerNewAction(ZoomIn, Constants::ACTION_ZOOM_IN, tr("Zoom In"),
                      QKeySequence(tr("Ctrl++")));
    registerNewAction(ZoomOut, Constants::ACTION_ZOOM_OUT, tr("Zoom Out"),
                      QKeySequence(tr("Ctrl+-")));
    registerNewAction(OriginalSize, Constants::ACTION_ORIGINAL_SIZE, tr("Original Size"),
                      QKeySequence(Core::UseMacShortcuts ? tr("Meta+0") : tr("Ctrl+0")));
    registerNewAction(FitToScreen, Constants::ACTION_FIT_TO_SCREEN, tr("Fit To Screen"),
                      QKeySequence(tr("Ctrl+=")));
    registerNewAction(Background, Constants::ACTION_BACKGROUND, tr("Switch Background"),
                      QKeySequence(tr("Ctrl+[")));
    registerNewAction(Outline, Constants::ACTION_OUTLINE, tr("Switch Outline"),
                      QKeySequence(tr("Ctrl+]")));
    registerNewAction(ToggleAnimation, Constants::ACTION_TOGGLE_ANIMATION, tr("Toggle Animation"),
                      QKeySequence());
}

/*!
    Creates a new action with the internal id \a actionId, command id \a id,
    and keyboard shortcut \a key, and registers it in the action manager.
*/

void ImageViewerActionHandler::registerNewAction(int actionId, Core::Id id,
    const QString &title, const QKeySequence &key)
{
    Core::Context context(Constants::IMAGEVIEWER_ID);
    QAction *action = new QAction(title, this);
    Core::Command *command = Core::ActionManager::registerAction(action, id, context);
    command->setDefaultKeySequence(key);
    connect(action, SIGNAL(triggered()), m_signalMapper, SLOT(map()));
    m_signalMapper->setMapping(action, actionId);
}

} // namespace Internal
} // namespace ImageViewer
