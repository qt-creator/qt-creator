/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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

#ifndef WATCHUTILS_H
#define WATCHUTILS_H

#include <QSet>
#include <QString>

namespace TextEditor {
    class ITextEditor;
}

namespace Core {
    class IEditor;
}

namespace CPlusPlus {
    class Snapshot;
}

namespace Debugger {
namespace Internal {

class WatchData;
class GdbMi;

// Keep in sync with dumper.py
enum DebuggerEncoding
{
    Unencoded8Bit                          =  0,
    Base64Encoded8BitWithQuotes            =  1,
    Base64Encoded16BitWithQuotes           =  2,
    Base64Encoded32BitWithQuotes           =  3,
    Base64Encoded16Bit                     =  4,
    Base64Encoded8Bit                      =  5,
    Hex2EncodedLatin1WithQuotes            =  6,
    Hex4EncodedLittleEndianWithQuotes      =  7,
    Hex8EncodedLittleEndianWithQuotes      =  8,
    Hex2EncodedUtf8WithQuotes              =  9,
    Hex8EncodedBigEndian                   = 10,
    Hex4EncodedBigEndianWithQuotes         = 11,
    Hex4EncodedLittleEndianWithoutQuotes   = 12,
    Hex2EncodedLocal8BitWithQuotes         = 13,
    JulianDate                             = 14,
    MillisecondsSinceMidnight              = 15,
    JulianDateAndMillisecondsSinceMidnight = 16,
    Hex2EncodedInt1                        = 17,
    Hex2EncodedInt2                        = 18,
    Hex2EncodedInt4                        = 19,
    Hex2EncodedInt8                        = 20,
    Hex2EncodedFloat4                      = 21,
    Hex2EncodedFloat8                      = 22
};

bool isEditorDebuggable(Core::IEditor *editor);
QByteArray dotEscape(QByteArray str);
QString currentTime();
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
bool isSymbianIntType(const QByteArray &type);

QString quoteUnprintableLatin1(const QByteArray &ba);

// Editor tooltip support
bool isCppEditor(Core::IEditor *editor);
QString cppExpressionAt(TextEditor::ITextEditor *editor, int pos,
                        int *line, int *column, QString *function = 0);
QString cppFunctionAt(const QString &fileName, int line);
// Decode string data as returned by the dumper helpers.
QString decodeData(const QByteArray &baIn, int encoding);
// Decode string data as returned by the dumper helpers.
void decodeArray(WatchData *list, const WatchData &tmplate,
    const QByteArray &rawData, int encoding);

// Get variables that are not initialized at a certain line
// of a function from the code model. Shadowed variables will
// be reported using the debugger naming conventions '<shadowed n>'
bool getUninitializedVariables(const CPlusPlus::Snapshot &snapshot,
   const QString &function, const QString &file, int line,
   QStringList *uninitializedVariables);


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
