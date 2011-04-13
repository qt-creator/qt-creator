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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CORELIB_GLOBAL_H
#define CORELIB_GLOBAL_H

// Unnecessary since core isn't a dll any more.

#define CORESHARED_EXPORT
#define TEST_CORESHARED_EXPORT

//#if defined(CORE_LIBRARY)
//#  define CORESHARED_EXPORT Q_DECL_EXPORT
//#else
//#  define CORESHARED_EXPORT Q_DECL_IMPORT
//#endif
//
//#if defined(TEST_EXPORTS)
//#if defined(CORE_LIBRARY)
//#  define TEST_CORESHARED_EXPORT Q_DECL_EXPORT
//#else
//#  define TEST_CORESHARED_EXPORT Q_DECL_IMPORT
//#endif
//#else
//#  define TEST_CORESHARED_EXPORT
//#endif

#include <QtCore/qglobal.h>

#endif // CORELIB_GLOBAL_H
