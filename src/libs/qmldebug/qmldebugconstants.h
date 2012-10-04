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

#ifndef QMLDEBUGCONSTANTS_H
#define QMLDEBUGCONSTANTS_H

namespace QmlDebug {
namespace Constants {

const char STR_WAITING_FOR_CONNECTION[] = "Waiting for connection ";
const char STR_ON_PORT_PATTERN[] = "on port (\\d+)";
const char STR_VIA_OST[] = "via OST";
const char STR_UNABLE_TO_LISTEN[] = "Unable to listen ";
const char STR_IGNORING_DEBUGGER[] = "Ignoring \"-qmljsdebugger=";
const char STR_IGNORING_DEBUGGER2[] = "Ignoring\"-qmljsdebugger="; // There is (was?) a bug in one of the error strings - safest to handle both
const char STR_CONNECTION_ESTABLISHED[] = "Connection established";

const char QDECLARATIVE_ENGINE[] = "QDeclarativeEngine";

} // namespace Constants
} // namespace QmlDebug

#endif // QMLDEBUGCONSTANTS_H
