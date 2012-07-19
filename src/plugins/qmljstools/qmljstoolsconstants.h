/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef QMLJSTOOLS_CONSTANTS_H
#define QMLJSTOOLS_CONSTANTS_H

#include <QtGlobal>

namespace QmlJSTools {
namespace Constants {

const char QML_MIMETYPE[] = "application/x-qml";
const char JS_MIMETYPE[] = "application/javascript";
const char JSON_MIMETYPE[] = "application/json";

const char TASK_INDEX[] = "QmlJSEditor.TaskIndex";

const char QML_JS_CODE_STYLE_SETTINGS_ID[] = "A.Code Style";
const char QML_JS_CODE_STYLE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("QmlJSTools", "Code Style");

const char QML_JS_SETTINGS_ID[] = "QmlJS";
const char QML_JS_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("QmlJSTools", "Qt Quick");

const char M_TOOLS_QMLJS[] = "QmlJSTools.Tools.Menu";
const char RESET_CODEMODEL[] = "QmlJSTools.ResetCodeModel";

} // namespace Constants
} // namespace QmlJSTools

#endif // QMLJSTOOLS_CONSTANTS_H
