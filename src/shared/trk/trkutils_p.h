/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_TRK_PRIVATE_UTILS
#define DEBUGGER_TRK_PRIVATE_UTILS

#include "trkutils.h"
#include "symbianutils_global.h"

QT_BEGIN_NAMESPACE
class QDateTime;
QT_END_NAMESPACE

namespace trk {

void appendDateTime(QByteArray *ba, QDateTime dateTime, Endianness = TargetByteOrder);
// returns a QByteArray containing optionally
// the serial frame [0x01 0x90 <len>] and 0x7e encoded7d(ba) 0x7e
QByteArray frameMessage(byte command, byte token, const QByteArray &data, bool serialFrame);
bool extractResult(QByteArray *buffer, bool serialFrame, TrkResult *r, QByteArray *rawData = 0);

} // namespace trk

#endif // DEBUGGER_TRK_PRIVATE_UTILS
