/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef IMAGEVIEWERCONSTANTS_H
#define IMAGEVIEWERCONSTANTS_H

namespace ImageViewer {
namespace Constants {

const char IMAGEVIEWER_ID[] = "Editors.ImageViewer";
const char IMAGEVIEWER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Image Viewer");

const char ACTION_ZOOM_IN[] = "ImageViewer.ZoomIn";
const char ACTION_ZOOM_OUT[] = "ImageViewer.ZoomOut";
const char ACTION_ORIGINAL_SIZE[] = "ImageViewer.OriginalSize";
const char ACTION_FIT_TO_SCREEN[] = "ImageViewer.FitToScreen";
const char ACTION_BACKGROUND[] = "ImageViewer.Background";
const char ACTION_OUTLINE[] = "ImageViewer.Outline";
const char ACTION_TOGGLE_ANIMATION[] = "ImageViewer.ToggleAnimation";

} // namespace Constants
} // namespace ImageViewer

#endif // IMAGEVIEWERCONSTANTS_H
