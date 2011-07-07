/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef QDECLARATIVEDEBUGHELPER_P_H
#define QDECLARATIVEDEBUGHELPER_P_H

#include "../qmljsdebugger_global.h"
#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QScriptEngine;
class QDeclarativeEngine;

// Helper methods to access private API through a stable interface
// This is used in the qmljsdebugger library of QtCreator.
class QMLJSDEBUGGER_EXTERN QDeclarativeDebugHelper
{
public:
    static QScriptEngine *getScriptEngine(QDeclarativeEngine *engine);
    static void setAnimationSlowDownFactor(qreal factor);

    // Enables remote debugging functionality
    // Only use this for debugging in a safe environment!
    static void enableDebugging();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVEDEBUGHELPER_P_H
