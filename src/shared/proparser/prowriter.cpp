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

#include "proitems.h"
#include "prowriter.h"

#include <QDir>
#include <QPair>

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
            case TokEnvVar:
                skipStr(tokPtr);
                break;
            case TokHashLiteral:
            case TokVariable:
            case TokProperty:
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
    case TokReturn:
    case TokNext:
    case TokBreak:
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

bool ProWriter::locateVarValues(const ushort *tokPtr,
    const QString &scope, const QString &var, int *scopeStart, int *bestLine)
{
    const bool inScope = scope.isEmpty();
    int lineNo = *scopeStart + 1;
    QString tmp;
    const ushort *lastXpr = 0;
    bool fresh = true;
    while (ushort tok = *tokPtr++) {
        if (inScope && (tok == TokAssign || tok == TokAppend || tok == TokAppendUnique)) {
            if (getLiteral(lastXpr, tokPtr - 1, tmp) && var == tmp) {
                *bestLine = lineNo - 1;
                return true;
            }
            skipExpression(++tokPtr, lineNo);
            fresh = true;
        } else {
            if (!inScope && tok == TokCondition && *tokPtr == TokBranch
                && getLiteral(lastXpr, tokPtr - 1, tmp) && scope == tmp) {
                *scopeStart = lineNo - 1;
                if (locateVarValues(tokPtr + 3, QString(), var, scopeStart, bestLine))
                    return true;
            }
            const ushort *oTokPtr = skipToken(tok, tokPtr, lineNo);
            if (tok != TokLine) {
                if (oTokPtr) {
                    if (fresh)
                        lastXpr = oTokPtr;
                } else if (tok == TokNot || tok == TokAnd || tok == TokOr) {
                    fresh = false;
                } else {
                    fresh = true;
                }
            }
        }
    }
    return false;
}

static int skipContLines(QStringList *lines, int lineNo, bool addCont)
{
    for (; lineNo < lines->count(); lineNo++) {
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
            if (addCont)
                (*lines)[lineNo].insert(line.length(), QLatin1String(" \\"));
            lineNo++;
            break;
        }
    }
    return lineNo;
}

void ProWriter::putVarValues(ProFile *profile, QStringList *lines,
    const QStringList &values, const QString &var, PutFlags flags, const QString &scope)
{
    QString indent = scope.isEmpty() ? QString() : QLatin1String("    ");
    int scopeStart = -1, lineNo;
    if (locateVarValues(profile->tokPtr(), scope, var, &scopeStart, &lineNo)) {
        if (flags & ReplaceValues) {
            // remove continuation lines with old values
            int lNo = skipContLines(lines, lineNo, false);
            lines->erase(lines->begin() + lineNo + 1, lines->begin() + lNo);
            // remove rest of the line
            QString &line = (*lines)[lineNo];
            int eqs = line.indexOf(QLatin1Char('='));
            if (eqs >= 0) // If this is not true, we mess up the file a bit.
                line.truncate(eqs + 1);
            // put new values
            foreach (const QString &v, values)
                line += ((flags & MultiLine) ? QLatin1String(" \\\n    ") + indent : QString::fromLatin1(" ")) + v;
        } else {
            lineNo = skipContLines(lines, lineNo, true);
            QString added;
            foreach (const QString &v, values)
                added += QLatin1String("    ") + indent + v + QLatin1String(" \\\n");
            added.chop(3);
            lines->insert(lineNo, added);
        }
    } else {
        // Create & append new variable item
        QString added;
        int lNo = lines->count();
        if (!scope.isEmpty()) {
            if (scopeStart < 0) {
                added = QLatin1Char('\n') + scope + QLatin1String(" {");
            } else {
                QRegExp rx(QLatin1String("(\\s*") + scope + QLatin1String("\\s*:\\s*)[^\\s{].*"));
                if (rx.exactMatch(lines->at(scopeStart))) {
                    (*lines)[scopeStart].replace(0, rx.cap(1).length(),
                                                 QString(scope + QLatin1String(" {\n    ")));
                    lNo = skipContLines(lines, scopeStart, false);
                    scopeStart = -1;
                }
            }
        }
        if (scopeStart >= 0) {
            lNo = scopeStart;
            int braces = 0;
            do {
                const QString &line = (*lines).at(lNo);
                for (int i = 0; i < line.size(); i++)
                    // This is pretty sick, but qmake does pretty much the same ...
                    if (line.at(i) == QLatin1Char('{')) {
                        ++braces;
                    } else if (line.at(i) == QLatin1Char('}')) {
                        if (!--braces)
                            break;
                    } else if (line.at(i) == QLatin1Char('#')) {
                        break;
                    }
            } while (braces && ++lNo < lines->size());
        }
        for (; lNo > scopeStart + 1 && lines->at(lNo - 1).isEmpty(); lNo--) ;
        if (lNo != scopeStart + 1)
            added += QLatin1Char('\n');
        added += indent + var + QLatin1String((flags & AppendOperator) ? " +=" : " =");
        foreach (const QString &v, values)
            added += ((flags & MultiLine) ? QLatin1String(" \\\n    ") + indent : QString::fromLatin1(" ")) + v;
        if (!scope.isEmpty() && scopeStart < 0)
            added += QLatin1String("\n}");
        lines->insert(lNo, added);
    }
}

void ProWriter::addFiles(ProFile *profile, QStringList *lines,
    const QDir &proFileDir, const QStringList &values, const QString &var)
{
    QStringList valuesToWrite;
    foreach (const QString &v, values)
        valuesToWrite << proFileDir.relativeFilePath(v);

    putVarValues(profile, lines, valuesToWrite, var, AppendValues | MultiLine | AppendOperator);
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
            if (blockLen) {
                findProVariables(tokPtr, vars, proVars, lineNo);
                tokPtr += blockLen;
            }
            blockLen = getBlockLen(tokPtr);
            if (blockLen) {
                findProVariables(tokPtr, vars, proVars, lineNo);
                tokPtr += blockLen;
            }
        } else if (tok == TokAssign || tok == TokAppend || tok == TokAppendUnique) {
            if (getLiteral(lastXpr, tokPtr - 1, tmp) && vars.contains(tmp))
                *proVars << lineNo;
            skipExpression(++tokPtr, lineNo);
        } else {
            lastXpr = skipToken(tok, tokPtr, lineNo);
        }
    }
}

QList<int> ProWriter::removeVarValues(ProFile *profile, QStringList *lines,
    const QStringList &values, const QStringList &vars)
{
    QList<int> notChanged;
    // yeah, this is a bit silly
    for (int i = 0; i < values.size(); i++)
        notChanged << i;

    QList<int> varLines;
    findProVariables(profile->tokPtr(), vars, &varLines);

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
                   const int pos = values.indexOf(fn);
                   if (pos != -1) {
                       notChanged.removeOne(pos);
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
                       chars = line.data();
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

QStringList ProWriter::removeFiles(ProFile *profile, QStringList *lines,
    const QDir &proFileDir, const QStringList &values, const QStringList &vars)
{
    // This is a tad stupid - basically, it can remove only entries which
    // the above code added.
    QStringList valuesToFind;
    foreach (const QString &absoluteFilePath, values)
        valuesToFind << proFileDir.relativeFilePath(absoluteFilePath);

    QStringList notChanged;
    foreach (int i, removeVarValues(profile, lines, valuesToFind, vars))
        notChanged.append(values.at(i));
    return notChanged;
}
