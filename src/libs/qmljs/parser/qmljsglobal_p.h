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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#ifdef QT_CREATOR
#  define QT_QML_BEGIN_NAMESPACE
#  define QT_QML_END_NAMESPACE

#  ifdef QMLJS_LIBRARY
#    define QML_PARSER_EXPORT Q_DECL_EXPORT
#  elif QML_BUILD_STATIC_LIB
#    define QML_PARSER_EXPORT
#  else
#    define QML_PARSER_EXPORT Q_DECL_IMPORT
#  endif // QMLJS_LIBRARY

#else // !QT_CREATOR
#  define QT_QML_BEGIN_NAMESPACE QT_BEGIN_NAMESPACE
#  define QT_QML_END_NAMESPACE QT_END_NAMESPACE
#  if defined(QT_BUILD_QMLDEVTOOLS_LIB) || defined(QT_QMLDEVTOOLS_LIB)
     // QmlDevTools is a static library
#    define QML_PARSER_EXPORT
#  elif defined(QT_BUILD_QML_LIB)
#    define QML_PARSER_EXPORT Q_DECL_EXPORT
#  else
#    define QML_PARSER_EXPORT Q_DECL_IMPORT
#  endif
#endif // QT_CREATOR

