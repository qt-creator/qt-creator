// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
#  elif QMLJS_STATIC_LIBRARY
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

