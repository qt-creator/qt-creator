/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef WATCHUTILS_H
#define WATCHUTILS_H

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include <QSet>
#include <QString>

namespace Debugger {
namespace Internal {

class WatchData;
class GdbMi;

bool isSkippableFunction(const QString &funcName, const QString &fileName);
bool isLeavableFunction(const QString &funcName, const QString &fileName);

bool hasLetterOrNumber(const QString &exp);
bool hasSideEffects(const QString &exp);
bool isKeyWord(const QString &exp);
bool isPointerType(const QByteArray &type);
bool isCharPointerType(const QByteArray &type);
bool startsWithDigit(const QString &str);
QByteArray stripPointerType(QByteArray type);
QByteArray gdbQuoteTypes(const QByteArray &type);
bool isFloatType(const QByteArray &type);
bool isIntOrFloatType(const QByteArray &type);
bool isIntType(const QByteArray &type);

QString formatToolTipAddress(quint64 a);
QString removeObviousSideEffects(const QString &exp);

// Decode string data as returned by the dumper helpers.
void decodeArray(WatchData *list, const WatchData &tmplate,
    const QByteArray &rawData, int encoding);


//
// GdbMi interaction
//

void setWatchDataValue(WatchData &data, const GdbMi &item);
void setWatchDataValueToolTip(WatchData &data, const GdbMi &mi,
    int encoding);
void setWatchDataChildCount(WatchData &data, const GdbMi &mi);
void setWatchDataValueEnabled(WatchData &data, const GdbMi &mi);
void setWatchDataAddress(WatchData &data, const GdbMi &addressMi, const GdbMi &origAddressMi);
void setWatchDataType(WatchData &data, const GdbMi &mi);
void setWatchDataDisplayedType(WatchData &data, const GdbMi &mi);

void parseWatchData(const QSet<QByteArray> &expandedINames,
    const WatchData &parent, const GdbMi &child,
    QList<WatchData> *insertions);

} // namespace Internal
} // namespace Debugger

#endif // WATCHUTILS_H
