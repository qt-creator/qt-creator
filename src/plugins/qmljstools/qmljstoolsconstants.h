/****************************************************************************
**
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
