/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
****************************************************************************/

#ifndef QMLPROFILEREXTENDED_GLOBAL_H
#define QMLPROFILEREXTENDED_GLOBAL_H

#include <QtGlobal>

#if defined(QMLPROFILEREXTENDED_LIBRARY)
#  define QMLPROFILEREXTENDEDSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QMLPROFILEREXTENDEDSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QMLPROFILEREXTENDED_GLOBAL_H

