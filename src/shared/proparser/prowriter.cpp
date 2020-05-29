/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmakeparser.h"
#include "prowriter.h"
#include "proitems.h"

#include <utils/algorithm.h>

#include <QDir>
#include <QLoggingCategory>
#include <QPair>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(prowriterLog, "qtc.prowriter", QtWarningMsg)

using namespace QmakeProjectManager::Internal;

static uint getBlockLen(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    len |= uint(*tokPtr++ << 16);
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
            Q_FALLTHROUGH();
        case TokLiteral: {
            int len = *tokPtr++;
            tmp.setRawData(reinterpret_cast<const QChar *>(tokPtr), len);
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
    for (;;) {
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
        Q_FALLTHROUGH();
    case TokTestCall:
        skipExpression(tokPtr, lineNo);
        break;
    case TokForLoop:
        skipHashStr(tokPtr);
        Q_FALLTHROUGH();
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
    return nullptr;
}

QString ProWriter::compileScope(const QString &scope)
{
    if (scope.isEmpty())
        return QString();
    QMakeParser parser(nullptr, nullptr, nullptr);
    ProFile *includeFile = parser.parsedProBlock(QStringRef(&scope), 0, "no-file", 1);
    if (!includeFile)
        return QString();
    const QString result = includeFile->items();
    includeFile->deref();
    return result.mid(2); // chop of TokLine + linenumber
}

static bool startsWithTokens(const ushort *that, const ushort *thatEnd, const ushort *s, const ushort *sEnd)
{
    if (thatEnd - that < sEnd - s)
        return false;

    do {
        if (*that != *s)
            return false;
        ++that;
        ++s;
    } while (s < sEnd);
    return true;
}

bool ProWriter::locateVarValues(const ushort *tokPtr, const ushort *tokPtrEnd,
    const QString &scope, const QString &var, int *scopeStart, int *bestLine)
{
    const bool inScope = scope.isEmpty();
    int lineNo = *scopeStart + 1;
    QString tmp;
    const ushort *lastXpr = nullptr;
    bool fresh = true;

    QString compiledScope = compileScope(scope);
    const ushort *cTokPtr = reinterpret_cast<const ushort *>(compiledScope.constData());

    while (ushort tok = *tokPtr++) {
        if (inScope && (tok == TokAssign || tok == TokAppend || tok == TokAppendUnique)) {
            if (getLiteral(lastXpr, tokPtr - 1, tmp) && var == tmp) {
                *bestLine = lineNo - 1;
                return true;
            }
            skipExpression(++tokPtr, lineNo);
            fresh = true;
        } else {
            if (!inScope && fresh
                    && startsWithTokens(tokPtr - 1, tokPtrEnd, cTokPtr, cTokPtr + compiledScope.size())
                    && *(tokPtr -1 + compiledScope.size()) == TokBranch) {
                *scopeStart = lineNo - 1;
                if (locateVarValues(tokPtr + compiledScope.size() + 2, tokPtrEnd,
                                    QString(), var, scopeStart, bestLine))
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

struct LineInfo
{
    QString indent;
    int continuationPos = 0;
    bool hasComment = false;
};

static LineInfo lineInfo(const QString &line)
{
    LineInfo li;
    li.continuationPos = line.length();
    const int idx = line.indexOf('#');
    li.hasComment = idx >= 0;
    if (li.hasComment)
        li.continuationPos = idx;
    for (int i = idx - 1; i >= 0 && (line.at(i) == ' ' || line.at(i) == '\t'); --i)
        --li.continuationPos;
    for (int i = 0; i < line.length() && (line.at(i) == ' ' || line.at(i) == '\t'); ++i)
        li.indent += line.at(i);
    return li;
}

struct ContinuationInfo {
    QString indent; // Empty means use default
    int lineNo;
};

static ContinuationInfo skipContLines(QStringList *lines, int lineNo, bool addCont)
{
    bool hasConsistentIndent = true;
    QString lastIndent;
    for (; lineNo < lines->count(); lineNo++) {
        const QString line = lines->at(lineNo);
        LineInfo li = lineInfo(line);
        if (hasConsistentIndent) {
            if (lastIndent.isEmpty())
                lastIndent = li.indent;
            else if (lastIndent != li.indent)
                hasConsistentIndent = false;
        }
        if (li.continuationPos == 0) {
            if (li.hasComment)
                continue;
            break;
        }
        if (line.at(li.continuationPos - 1) != '\\') {
            if (addCont)
                (*lines)[lineNo].insert(li.continuationPos, " \\");
            lineNo++;
            break;
        }
    }
    ContinuationInfo ci;
    if (hasConsistentIndent)
        ci.indent = lastIndent;
    ci.lineNo = lineNo;
    return ci;
}

void ProWriter::putVarValues(ProFile *profile, QStringList *lines, const QStringList &values,
                             const QString &var, PutFlags flags, const QString &scope,
                             const QString &continuationIndent)
{
    qCDebug(prowriterLog) << Q_FUNC_INFO << "lines:" << *lines << "values:" << values
                          << "var:" << var << "flags:" << int(flags) << "scope:" << scope
                          << "indent:" << continuationIndent;
    const QString indent = scope.isEmpty() ? QString() : continuationIndent;
    const auto effectiveContIndent = [indent, continuationIndent](const ContinuationInfo &ci) {
        return !ci.indent.isEmpty() ? ci.indent : continuationIndent + indent;
    };
    int scopeStart = -1, lineNo;
    if (locateVarValues(profile->tokPtr(), profile->tokPtrEnd(), scope, var, &scopeStart, &lineNo)) {
        if (flags & ReplaceValues) {
            // remove continuation lines with old values
            const ContinuationInfo contInfo = skipContLines(lines, lineNo, false);
            lines->erase(lines->begin() + lineNo + 1, lines->begin() + contInfo.lineNo);
            // remove rest of the line
            QString &line = (*lines)[lineNo];
            int eqs = line.indexOf('=');
            if (eqs >= 0) // If this is not true, we mess up the file a bit.
                line.truncate(eqs + 1);
            // put new values
            qCDebug(prowriterLog) << 1 << "old line value:" << line;
            for (const QString &v : values) {
                line += ((flags & MultiLine) ? QString(" \\\n") + effectiveContIndent(contInfo)
                                             : QString(" ")) + v;
            }
            qCDebug(prowriterLog) << "new line value:" << line;
        } else {
            const ContinuationInfo contInfo = skipContLines(lines, lineNo, false);
            int endLineNo = contInfo.lineNo;
            for (const QString &v : values) {
                int curLineNo = lineNo + 1;
                while (curLineNo < endLineNo && v >= lines->at(curLineNo).trimmed())
                    ++curLineNo;
                QString newLine = effectiveContIndent(contInfo) + v;
                if (curLineNo == endLineNo) {
                    QString &oldLastLine = (*lines)[endLineNo - 1];
                    if (!oldLastLine.endsWith('\\'))
                        oldLastLine.insert(lineInfo(oldLastLine).continuationPos, " \\");
                } else {
                    newLine += " \\";
                }
                qCDebug(prowriterLog) << 2 << "adding new line" << newLine
                                      << "at line " << curLineNo;
                lines->insert(curLineNo, newLine);
                ++endLineNo;
            }
        }
    } else {
        // Create & append new variable item
        QString added;
        int lNo = lines->count();
        ContinuationInfo contInfo;
        if (!scope.isEmpty()) {
            if (scopeStart < 0) {
                added = '\n' + scope + " {";
            } else {
                // TODO use anchoredPattern() once Qt 5.12 is mandatory
                const QRegularExpression rx("\\A(\\s*" + scope + "\\s*:\\s*)[^\\s{].*\\z");
                const QRegularExpressionMatch match(rx.match(lines->at(scopeStart)));
                if (match.hasMatch()) {
                    qCDebug(prowriterLog) << 3 << "old line value:" << (*lines)[scopeStart];
                    (*lines)[scopeStart].replace(0, match.captured(1).length(),
                                                 scope + " {\n" + continuationIndent);
                    qCDebug(prowriterLog) << "new line value:" << (*lines)[scopeStart];
                    contInfo = skipContLines(lines, scopeStart, false);
                    lNo = contInfo.lineNo;
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
                    if (line.at(i) == '{') {
                        ++braces;
                    } else if (line.at(i) == '}') {
                        if (!--braces)
                            break;
                    } else if (line.at(i) == '#') {
                        break;
                    }
            } while (braces && ++lNo < lines->size());
        }
        for (; lNo > scopeStart + 1 && lines->at(lNo - 1).isEmpty(); lNo--) ;
        if (lNo != scopeStart + 1)
            added += '\n';
        added += indent + var + ((flags & AppendOperator) ? " +=" : " =");
        for (const QString &v : values) {
            added += ((flags & MultiLine) ? QString(" \\\n") + effectiveContIndent(contInfo)
                                          : QString(" ")) + v;
        }
        if (!scope.isEmpty() && scopeStart < 0)
            added += "\n}";
        qCDebug(prowriterLog) << 4 << "adding" << added << "at line" << lNo;
        lines->insert(lNo, added);
    }
}

void ProWriter::addFiles(ProFile *profile, QStringList *lines, const QStringList &values,
                         const QString &var, const QString &continuationIndent)
{
    QStringList valuesToWrite;
    QString prefixPwd;
    QDir baseDir = QFileInfo(profile->fileName()).absoluteDir();
    if (profile->fileName().endsWith(".pri"))
        prefixPwd = "$$PWD/";
    for (const QString &v : values)
        valuesToWrite << (prefixPwd + baseDir.relativeFilePath(v));

    putVarValues(profile, lines, valuesToWrite, var, AppendValues | MultiLine | AppendOperator,
                 QString(), continuationIndent);
}

static void findProVariables(const ushort *tokPtr, const QStringList &vars,
                             ProWriter::VarLocations &proVars, const uint firstLine = 0)
{
    int lineNo = firstLine;
    QString tmp;
    const ushort *lastXpr = nullptr;
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
            if (getLiteral(lastXpr, tokPtr - 1, tmp) && vars.contains(tmp)) {
                QString varName = tmp;
                varName.detach(); // tmp was constructed via setRawData()
                proVars << qMakePair(varName, lineNo);
            }
            skipExpression(++tokPtr, lineNo);
        } else {
            lastXpr = skipToken(tok, tokPtr, lineNo);
        }
    }
}

QList<int> ProWriter::removeVarValues(ProFile *profile, QStringList *lines,
    const QStringList &values, const QStringList &vars, VarLocations *removedLocations)
{
    QList<int> notChanged;
    // yeah, this is a bit silly
    for (int i = 0; i < values.size(); i++)
        notChanged << i;

    VarLocations varLocations;
    findProVariables(profile->tokPtr(), vars, varLocations);

    // This code expects proVars to be sorted by the variables' appearance in the file.
    int delta = 1;
    for (int varIndex = 0; varIndex < varLocations.count(); ++varIndex) {
       const VarLocation &loc = varLocations[varIndex];
       bool first = true;
       int lineNo = loc.second - delta;
       typedef QPair<int, int> ContPos;
       QList<ContPos> contPos;
       const auto nextSegmentStart = [varIndex, lines, &delta, &varLocations] {
           return varIndex == varLocations.count() - 1
                   ? lines->count() : varLocations[varIndex + 1].second - delta;
       };
       while (lineNo < nextSegmentStart()) {
           QString &line = (*lines)[lineNo];
           int lineLen = line.length();
           bool killed = false;
           bool saved = false;
           int idx = line.indexOf('#');
           if (idx >= 0)
               lineLen = idx;
           QChar *chars = line.data();
           for (;;) {
               if (!lineLen) {
                   if (idx >= 0)
                       goto nextLine;
                   goto nextVar;
               }
               QChar c = chars[lineLen - 1];
               if (c != ' ' && c != '\t')
                   break;
               lineLen--;
           }
           {
               int contCol = -1;
               if (chars[lineLen - 1] == '\\')
                   contCol = --lineLen;
               int colNo = 0;
               if (first) {
                   colNo = line.indexOf('=') + 1;
                   first = false;
                   saved = true;
               }
               while (colNo < lineLen) {
                   QChar c = chars[colNo];
                   if (c == ' ' || c == '\t') {
                       colNo++;
                       continue;
                   }
                   int varCol = colNo;
                   while (colNo < lineLen) {
                       QChar c = chars[colNo];
                       if (c == (' ') || c == ('\t'))
                           break;
                       colNo++;
                   }
                   const QString fn = line.mid(varCol, colNo - varCol);
                   const int pos = values.indexOf(fn);
                   if (pos != -1) {
                       notChanged.removeOne(pos);
                       if (removedLocations)
                           *removedLocations << qMakePair(loc.first, loc.second - delta);
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
                           line.insert(idx, "# " + fn + ' ');
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
                       for (const ContPos &pos : qAsConst(contPos)) {
                           QString &bline = (*lines)[pos.first];
                           bline.remove(pos.second, 1);
                           if (pos.second == bline.length())
                               while (bline.endsWith(' ') || bline.endsWith('\t'))
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

QStringList ProWriter::removeFiles(
        ProFile *profile,
        QStringList *lines,
        const QDir &proFileDir,
        const QStringList &values,
        const QStringList &vars,
        VarLocations *removedLocations)
{
    // This is a tad stupid - basically, it can remove only entries which
    // the above code added.
    QStringList valuesToFind;
    for (const QString &absoluteFilePath : values)
        valuesToFind << proFileDir.relativeFilePath(absoluteFilePath);

    const QStringList notYetChanged =
            Utils::transform(removeVarValues(profile, lines, valuesToFind, vars, removedLocations),
                             [values](int i) { return values.at(i); });

    if (!profile->fileName().endsWith(".pri"))
        return notYetChanged;

    // If we didn't find them with a relative path to the .pro file
    // maybe those files can be found via $$PWD/relativeToPriFile

    valuesToFind.clear();
    const QDir baseDir = QFileInfo(profile->fileName()).absoluteDir();
    const QString prefixPwd = "$$PWD/";
    for (const QString &absoluteFilePath : notYetChanged)
        valuesToFind << (prefixPwd + baseDir.relativeFilePath(absoluteFilePath));

    const QStringList notChanged =
            Utils::transform(removeVarValues(profile, lines, valuesToFind, vars, removedLocations),
                             [notYetChanged](int i) { return notYetChanged.at(i); });

    return notChanged;
}
