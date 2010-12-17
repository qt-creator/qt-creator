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

#ifndef IMAGEVIEWERCONSTANTS_H
#define IMAGEVIEWERCONSTANTS_H

namespace ImageViewer {
namespace Constants {

const char * const IMAGEVIEWER_ID = "Editors.ImageViewer";
const char * const IMAGEVIEWER_DISPLAY_NAME = QT_TRANSLATE_NOOP("OpenWith::Editors", "Image Viewer");

// actions
const char * const ACTION_ZOOM_IN = "ImageViewer.ZoomIn";
const char * const ACTION_ZOOM_OUT = "ImageViewer.ZoomOut";
const char * const ACTION_ORIGINAL_SIZE = "ImageViewer.OriginalSize";
const char * const ACTION_FIT_TO_SCREEN = "ImageViewer.FitToScreen";
const char * const ACTION_BACKGROUND = "ImageViewer.Background";
const char * const ACTION_OUTLINE = "ImageViewer.Outline";

} // namespace Constants
} // namespace ImageViewer

#endif // IMAGEVIEWERCONSTANTS_H
