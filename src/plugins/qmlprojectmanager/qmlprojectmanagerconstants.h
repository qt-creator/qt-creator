/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <qglobal.h>

namespace QmlProjectManager {
namespace Constants {

const char * const QML_RC_ID("QmlProjectManager.QmlRunConfiguration");
const char * const QML_RC_DISPLAY_NAME(QT_TRANSLATE_NOOP("QmlProjectManager::Internal::QmlRunConfiguration", "QML Viewer"));
const char * const QML_VIEWER_KEY("QmlProjectManager.QmlRunConfiguration.QDeclarativeViewer");
const char * const QML_VIEWER_ARGUMENTS_KEY("QmlProjectManager.QmlRunConfiguration.QDeclarativeViewerArguments");
const char * const QML_VIEWER_TARGET_ID("QmlProjectManager.QmlTarget");
const char * const QML_VIEWER_TARGET_DISPLAY_NAME("QML Viewer");
const char * const QML_MAINSCRIPT_KEY("QmlProjectManager.QmlRunConfiguration.MainScript");
const char * const QML_DEBUG_SERVER_ADDRESS_KEY("QmlProjectManager.QmlRunConfiguration.DebugServerAddress");
const char * const QML_DEBUG_SERVER_PORT_KEY("QmlProjectManager.QmlRunConfiguration.DebugServerPort");

const int QML_DEFAULT_DEBUG_SERVER_PORT(3768);

} // namespace Constants
} // namespace QmlProjectManager
