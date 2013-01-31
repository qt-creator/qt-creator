/**************************************************************************
**
** Copyright (C) 2013 Denis Mingulov.
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "imagevieweractionhandler.h"
#include "imageviewer.h"
#include "imageviewerconstants.h"

#include <QList>
#include <QSignalMapper>
#include <QAction>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/id.h>

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

void ImageViewerActionHandler::registerNewAction(int actionId, const Core::Id &id,
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
