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

#pragma once

namespace ImageViewer {
namespace Constants {

const char IMAGEVIEWER_ID[] = "Editors.ImageViewer";
const char IMAGEVIEWER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Image Viewer");

const char ACTION_EXPORT_IMAGE[] = "ImageViewer.ExportImage";
const char ACTION_ZOOM_IN[] = "ImageViewer.ZoomIn";
const char ACTION_ZOOM_OUT[] = "ImageViewer.ZoomOut";
const char ACTION_ORIGINAL_SIZE[] = "ImageViewer.OriginalSize";
const char ACTION_FIT_TO_SCREEN[] = "ImageViewer.FitToScreen";
const char ACTION_BACKGROUND[] = "ImageViewer.Background";
const char ACTION_OUTLINE[] = "ImageViewer.Outline";
const char ACTION_TOGGLE_ANIMATION[] = "ImageViewer.ToggleAnimation";

} // namespace Constants
} // namespace ImageViewer
