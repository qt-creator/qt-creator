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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
