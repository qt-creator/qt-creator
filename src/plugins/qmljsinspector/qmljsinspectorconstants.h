/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJSINSPECTORCONSTANTS_H
#define QMLJSINSPECTORCONSTANTS_H

namespace QmlJSInspector {
namespace Constants {

const char INFO_EXPERIMENTAL[] = "QmlInspector.Experimental";
const char INFO_OUT_OF_SYNC[] = "QmlInspector.OutOfSyncWarning";

const char PLAY_ACTION[] = "QmlInspector.Play";
const char SELECT_ACTION[] = "QmlInspector.Select";
const char ZOOM_ACTION[] = "QmlInspector.Zoom";
const char COLOR_PICKER_ACTION[] = "QmlInspector.ColorPicker";
const char FROM_QML_ACTION[] = "QmlInspector.FromQml";
const char SHOW_APP_ON_TOP_ACTION[] = "QmlInspector.ShowAppOnTop";

// Settings
const char S_QML_INSPECTOR   [] = "QML.Inspector";
const char S_LIVE_PREVIEW_WARNING_KEY[] = "ShowLivePreview";

enum DesignTool {
    NoTool = 0,
    SelectionToolMode = 1,
    MarqueeSelectionToolMode = 2,
    MoveToolMode = 3,
    ResizeToolMode = 4,
    ColorPickerMode = 5,
    ZoomMode = 6
};

} // namespace Constants
} // namespace QmlJSInspector

#endif // QMLJSINSPECTORCONSTANTS_H
