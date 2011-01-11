/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "proitems.h"
#include "prowriter.h"

#include <QtCore/QDir>
#include <QtCore/QPair>

using namespace Qt4ProjectManager::Internal;

static uint getBlockLen(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    len |= (uint)*tokPtr++ << 16;
    return len;
}

static bool getLiteral(const ushort *tokPtr, const ushort *tokEnd, QString &tmp)
{
    int count = 0;
    while (tokPtr != tokEnd) {
        ushort tok = *tokPtr++;
        switch (tok & TokMask) {
        case TokLine:
            tokPtr++;
            break;
        case TokHashLiteral:
            tokPtr += 2;
            // fallthrough
        case TokLiteral: {
            uint len = *tokPtr++;
            tmp.setRawData((const QChar *)tokPtr, len);
            count++;
            tokPtr += len;
            break; }
        default:
            return false;
        }
    }
    return count == 1;
}

static void skipStr(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    tokPtr += len;
}

static void skipHashStr(const ushort *&tokPtr)
{
    tokPtr += 2;
    uint len = *tokPtr++;
    tokPtr += len;
}

static void skipBlock(const ushort *&tokPtr)
{
    uint len = getBlockLen(tokPtr);
    tokPtr += len;
}

static void skipExpression(const ushort *&pTokPtr, int &lineNo)
{
    const ushort *tokPtr = pTokPtr;
    forever {
        ushort tok = *tokPtr++;
        switch (tok) {
        case TokLine:
            lineNo = *tokPtr++;
            break;
        case TokValueTerminator:
        case TokFuncTerminator:
            pTokPtr = tokPtr;
            return;
        case TokArgSeparator:
            break;
        default:
            switch (tok & TokMask) {
            case TokLiteral:
            case TokProperty:
            case TokEnvVar:
                skipStr(tokPtr);
                break;
            case TokHashLiteral:
            case TokVariable:
                skipHashStr(tokPtr);
                break;
            case TokFuncName:
                skipHashStr(tokPtr);
                pTokPtr = tokPtr;
                skipExpression(pTokPtr, lineNo);
                tokPtr = pTokPtr;
                break;
            default:
                pTokPtr = tokPtr - 1;
                return;
            }
        }
    }
}

static const ushort *skipToken(ushort tok, const ushort *&tokPtr, int &lineNo)
{
    switch (tok) {
    case TokLine:
        lineNo = *tokPtr++;
        break;
    case TokAssign:
    case TokAppend:
    case TokAppendUnique:
    case TokRemove:
    case TokReplace:
        tokPtr++;
        // fallthrough
    case TokTestCall:
        skipExpression(tokPtr, lineNo);
        break;
    case TokForLoop:
        skipHashStr(tokPtr);
        // fallthrough
    case TokBranch:
        skipBlock(tokPtr);
        skipBlock(tokPtr);
        break;
    case TokTestDef:
    case TokReplaceDef:
        skipHashStr(tokPtr);
        skipBlock(tokPtr);
        break;
    case TokNot:
    case TokAnd:
    case TokOr:
    case TokCondition:
        break;
    default: {
            const ushort *oTokPtr = --tokPtr;
            skipExpression(tokPtr, lineNo);
            if (tokPtr != oTokPtr)
                return oTokPtr;
        }
        Q_ASSERT_X(false, "skipToken", "unexpected item type");
    }
    return 0;
}

void ProWriter::addVarValues(ProFile *profile, QStringList *lines,
    const QDir &proFileDir, const QStringList &values, const QString &var,
    bool valuesAreFiles)
{
    QStringList valuesToWrite;
    if (valuesAreFiles) {
        foreach (const QString &v, values)
            valuesToWrite << proFileDir.relativeFilePath(v);
    } else {
        valuesToWrite = values;
    }

    // Check if variable item exists as child of root item
    const ushort *tokPtr = profile->tokPtr();
    int lineNo = 0;
    QString tmp;
    const ushort *lastXpr = 0;
    while (ushort tok = *tokPtr++) {
        if (tok == TokAssign || tok == TokAppend || tok == TokAppendUnique) {
            if (getLiteral(lastXpr, tokPtr - 1, tmp) && var == tmp) {
                for (--lineNo; lineNo < lines->count(); lineNo++) {
                    QString line = lines->at(lineNo);
                    int idx = line.indexOf(QLatin1Char('#'));
                    if (idx >= 0)
                        line.truncate(idx);
                    while (line.endsWith(QLatin1Char(' ')) || line.endsWith(QLatin1Char('\t')))
                        line.chop(1);
                    if (line.isEmpty()) {
                        if (idx >= 0)
                            continue;
                        break;
                    }
                    if (!line.endsWith(QLatin1Char('\\'))) {
                        (*lines)[lineNo].insert(line.length(), QLatin1String(" \\"));
                        lineNo++;
                        break;
                    }
                }
                QString added;
                foreach (const QString &v, valuesToWrite)
                    added += QLatin1String("    ") + v + QLatin1String(" \\\n");
                added.chop(3);
                lines->insert(lineNo, added);
                return;
            }
            skipExpression(++tokPtr, lineNo);
        } else {
            lastXpr = skipToken(tok, tokPtr, lineNo);
        }
    }

    // Create & append new variable item
    QString added = QLatin1Char('\n') + var + QLatin1String(" +=");
    foreach (const QString &v, valuesToWrite)
        added += QLatin1String(" \\\n    ") + v;
    *lines << added;
}

static void findProVariables(const ushort *tokPtr, const QStringList &vars,
                             QList<int> *proVars, const uint firstLine = 0)
{
    int lineNo = firstLine;
    QString tmp;
    const ushort *lastXpr = 0;
    while (ushort tok = *tokPtr++) {
        if (tok == TokBranch) {
            uint blockLen = getBlockLen(tokPtr);
            findProVariables(tokPtr, vars, proVars, lineNo);
            tokPtr += blockLen;
            blockLen = getBlockLen(tokPtr);
            findProVariables(tokPtr, vars, proVars, lineNo);
            tokPtr += blockLen;
        } else if (tok == TokAssign || tok == TokAppend || tok == TokAppendUnique) {
            if (getLiteral(lastXpr, tokPtr - 1, tmp) && vars.contains(tmp))
                *proVars << lineNo;
            skipExpression(++tokPtr, lineNo);
        } else {
            lastXpr = skipToken(tok, tokPtr, lineNo);
        }
    }
}

QStringList ProWriter::removeVarValues(ProFile *profile, QStringList *lines,
    const QDir &proFileDir, const QStringList &values, const QStringList &vars,
    bool valuesAreFiles)
{
    QStringList notChanged = values;

    QList<int> varLines;
    findProVariables(profile->tokPtr(), vars, &varLines);

    QStringList valuesToFind;
    if (valuesAreFiles) {
        // This is a tad stupid - basically, it can remove only entries which
        // the above code added.
        foreach (const QString &absoluteFilePath, values)
            valuesToFind << proFileDir.relativeFilePath(absoluteFilePath);
    } else {
        valuesToFind = values;
    }

    // This code expects proVars to be sorted by the variables' appearance in the file.
    int delta = 1;
    foreach (int ln, varLines) {
       bool first = true;
       int lineNo = ln - delta;
       typedef QPair<int, int> ContPos;
       QList<ContPos> contPos;
       while (lineNo < lines->count()) {
           QString &line = (*lines)[lineNo];
           int lineLen = line.length();
           bool killed = false;
           bool saved = false;
           int idx = line.indexOf(QLatin1Char('#'));
           if (idx >= 0)
               lineLen = idx;
           QChar *chars = line.data();
           forever {
               if (!lineLen) {
                   if (idx >= 0)
                       goto nextLine;
                   goto nextVar;
               }
               QChar c = chars[lineLen - 1];
               if (c != QLatin1Char(' ') && c != QLatin1Char('\t'))
                   break;
               lineLen--;
           }
           {
               int contCol = -1;
               if (chars[lineLen - 1] == QLatin1Char('\\'))
                   contCol = --lineLen;
               int colNo = 0;
               if (first) {
                   colNo = line.indexOf(QLatin1Char('=')) + 1;
                   first = false;
                   saved = true;
               }
               while (colNo < lineLen) {
                   QChar c = chars[colNo];
                   if (c == QLatin1Char(' ') || c == QLatin1Char('\t')) {
                       colNo++;
                       continue;
                   }
                   int varCol = colNo;
                   while (colNo < lineLen) {
                       QChar c = chars[colNo];
                       if (c == QLatin1Char(' ') || c == QLatin1Char('\t'))
                           break;
                       colNo++;
                   }
                   const QString fn = line.mid(varCol, colNo - varCol);
                   const int pos = valuesToFind.indexOf(fn);
                   if (pos != -1) {
                       notChanged.removeOne(values.at(pos));
                       if (colNo < lineLen)
                           colNo++;
                       else if (varCol)
                           varCol--;
                       int len = colNo - varCol;
                       colNo = varCol;
                       line.remove(varCol, len);
                       lineLen -= len;
                       contCol -= len;
                       idx -= len;
                       if (idx >= 0)
                           line.insert(idx, QLatin1String("# ") + fn + QLatin1Char(' '));
                       killed = true;
                   } else {
                       saved = true;
                   }
               }
               if (saved) {
                   // Entries remained
                   contPos.clear();
               } else if (killed) {
                   // Entries existed, but were all removed
                   if (contCol < 0) {
                       // This is the last line, so clear continuations leading to it
                       foreach (const ContPos &pos, contPos) {
                           QString &bline = (*lines)[pos.first];
                           bline.remove(pos.second, 1);
                           if (pos.second == bline.length())
                               while (bline.endsWith(QLatin1Char(' '))
                                      || bline.endsWith(QLatin1Char('\t')))
                                   bline.chop(1);
                       }
                       contPos.clear();
                   }
                   if (idx < 0) {
                       // Not even a comment stayed behind, so zap the line
                       lines->removeAt(lineNo);
                       delta++;
                       continue;
                   }
               }
               if (contCol >= 0)
                   contPos.append(qMakePair(lineNo, contCol));
           }
         nextLine:
           lineNo++;
       }
     nextVar: ;
    }
    return notChanged;
}
