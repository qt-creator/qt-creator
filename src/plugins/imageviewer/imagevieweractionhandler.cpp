/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "imagevieweractionhandler.h"
#include "imageviewer.h"
#include "imageviewerconstants.h"

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QSignalMapper>
#include <QtGui/QAction>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/uniqueidmanager.h>

namespace ImageViewer {
namespace Internal {

enum SupportedActions { ZoomIn = 0, ZoomOut, OriginalSize, FitToScreen, Background, Outline };

struct ImageViewerActionHandlerPrivate
{
    ImageViewerActionHandlerPrivate()
        : context(Constants::IMAGEVIEWER_ID)
    {}

    QPointer<QAction> actionZoomIn;
    QPointer<QAction> actionZoomOut;
    QPointer<QAction> actionOriginalSize;
    QPointer<QAction> actionFitToScreen;
    QPointer<QAction> actionBackground;
    QPointer<QAction> actionOutline;

    Core::Context context;
    QPointer<QSignalMapper> signalMapper;
};

ImageViewerActionHandler::ImageViewerActionHandler(QObject *parent) :
    QObject(parent),
    d_ptr(new ImageViewerActionHandlerPrivate)
{
    d_ptr->signalMapper = new QSignalMapper(this);
    connect(d_ptr->signalMapper, SIGNAL(mapped(int)), SLOT(actionTriggered(int)));
}

ImageViewerActionHandler::~ImageViewerActionHandler()
{
}

void ImageViewerActionHandler::actionTriggered(int supportedAction)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->currentEditor();
    ImageViewer *viewer = qobject_cast<ImageViewer *>(editor);
    if (!viewer)
        return;

    SupportedActions action = static_cast<SupportedActions>(supportedAction);
    switch(action) {
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
    default:
        break;
    }
}

void ImageViewerActionHandler::createActions()
{
    registerNewAction(ZoomIn, Constants::ACTION_ZOOM_IN, tr("Zoom In"),
                      d_ptr->context, QKeySequence(tr("Ctrl++")));
    registerNewAction(ZoomOut, Constants::ACTION_ZOOM_OUT, tr("Zoom Out"),
                      d_ptr->context, QKeySequence(tr("Ctrl+-")));
    registerNewAction(OriginalSize, Constants::ACTION_ORIGINAL_SIZE, tr("Original Size"),
                      d_ptr->context, QKeySequence(tr("Ctrl+0")));
    registerNewAction(FitToScreen, Constants::ACTION_FIT_TO_SCREEN, tr("Fit To Screen"),
                      d_ptr->context, QKeySequence(tr("Ctrl+=")));
    registerNewAction(Background, Constants::ACTION_BACKGROUND, tr("Switch background"),
                      d_ptr->context, QKeySequence(tr("Ctrl+[")));
    registerNewAction(Outline, Constants::ACTION_OUTLINE, tr("Switch outline"),
                      d_ptr->context, QKeySequence(tr("Ctrl+]")));
}

QAction *ImageViewerActionHandler::registerNewAction(int actionId, const QString &id,
                           const QString &title, const Core::Context &context, const QKeySequence &key)
{
    Core::ActionManager *actionManager = Core::ICore::instance()->actionManager();
    Core::Command *command = 0;

    QAction *action = new QAction(title, this);
    command = actionManager->registerAction(action, id, context);
    if (command)
        command->setDefaultKeySequence(key);
    connect(action, SIGNAL(triggered()), d_ptr->signalMapper, SLOT(map()));
    d_ptr->signalMapper->setMapping(action, actionId);

    return action;
}

} // namespace Internal
} // namespace ImageViewer
