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
