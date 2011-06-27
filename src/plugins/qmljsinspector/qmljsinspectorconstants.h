/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#ifndef QMLJSINSPECTORCONSTANTS_H
#define QMLJSINSPECTORCONSTANTS_H

namespace QmlJSInspector {
namespace Constants {

const char * const INFO_EXPERIMENTAL = "QmlInspector.Experimental";
const char * const INFO_OUT_OF_SYNC = "QmlInspector.OutOfSyncWarning";

const char * const PLAY_ACTION = "QmlInspector.Play";
const char * const SELECT_ACTION = "QmlInspector.Select";
const char * const ZOOM_ACTION = "QmlInspector.Zoom";
const char * const COLOR_PICKER_ACTION = "QmlInspector.ColorPicker";
const char * const FROM_QML_ACTION = "QmlInspector.FromQml";
const char * const SHOW_APP_ON_TOP_ACTION = "QmlInspector.ShowAppOnTop";

// settings
const char * const S_QML_INSPECTOR    = "QML.Inspector";
const char * const S_LIVE_PREVIEW_WARNING_KEY = "ShowLivePreview";

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
} // namespace Qml

#endif
