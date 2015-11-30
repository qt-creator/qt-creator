/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSTOOLS_CONSTANTS_H
#define QMLJSTOOLS_CONSTANTS_H

#include <QtGlobal>

namespace QmlJSTools {
namespace Constants {

const char QML_MIMETYPE[] = "text/x-qml"; // separate def also in projectexplorerconstants.h
const char QBS_MIMETYPE[] = "application/x-qt.qbs+qml";
const char QMLPROJECT_MIMETYPE[] = "application/x-qmlproject";
const char QMLTYPES_MIMETYPE[] = "application/x-qt.meta-info+qml";
const char QMLUI_MIMETYPE[] = "application/x-qt.ui+qml";
const char JS_MIMETYPE[] = "application/javascript";
const char JSON_MIMETYPE[] = "application/json";

const char QML_JS_CODE_STYLE_SETTINGS_ID[] = "A.Code Style";
const char QML_JS_CODE_STYLE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("QmlJSTools", "Code Style");

const char QML_JS_SETTINGS_ID[] = "QmlJS";
const char QML_JS_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("QmlJSTools", "Qt Quick");

const char M_TOOLS_QMLJS[] = "QmlJSTools.Tools.Menu";
const char RESET_CODEMODEL[] = "QmlJSTools.ResetCodeModel";

const char SETTINGS_CATEGORY_QML_ICON[] = ":/qmljstools/images/category_qml.png";

} // namespace Constants
} // namespace QmlJSTools

#endif // QMLJSTOOLS_CONSTANTS_H
