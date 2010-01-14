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

#include "proitems.h"
#include "prowriter.h"

#include <QtCore/QDir>
#include <QtCore/QPair>

using namespace Qt4ProjectManager::Internal;

void ProWriter::addFiles(ProFile *profile, QStringList *lines,
                         const QDir &proFileDir, const QStringList &filePaths,
                         const QStringList &vars)
{
    // Check if variable item exists as child of root item
    foreach (ProItem *item, profile->items()) {
        if (item->kind() == ProItem::BlockKind) {
            ProBlock *block = static_cast<ProBlock *>(item);
            if (block->blockKind() == ProBlock::VariableKind) {
                ProVariable *proVar = static_cast<ProVariable*>(block);
                if (vars.contains(proVar->variable())
                    && proVar->variableOperator() != ProVariable::RemoveOperator
                    && proVar->variableOperator() != ProVariable::ReplaceOperator) {

                    int lineNo = proVar->lineNumber() - 1;
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
                            (*lines)[lineNo].insert(line.length(), QLatin1String(" \\"));
                            lineNo++;
                            break;
                        }
                    }
                    QString added;
                    foreach (const QString &filePath, filePaths)
                        added += QLatin1String("    ") + proFileDir.relativeFilePath(filePath)
                                 + QLatin1String(" \\\n");
                    added.chop(3);
                    lines->insert(lineNo, added);
                    return;
                }
            }
        }
    }

    // Create & append new variable item
    QString added = QLatin1Char('\n') + vars.first() + QLatin1String(" +=");
    foreach (const QString &filePath, filePaths)
        added += QLatin1String(" \\\n    ") + proFileDir.relativeFilePath(filePath);
    *lines << added;
}

static void findProVariables(ProBlock *block, const QStringList &vars,
                             QList<ProVariable *> *proVars)
{
    foreach (ProItem *item, block->items()) {
        if (item->kind() == ProItem::BlockKind) {
            ProBlock *subBlock = static_cast<ProBlock *>(item);
            if (subBlock->blockKind() == ProBlock::VariableKind) {
                ProVariable *proVar = static_cast<ProVariable*>(subBlock);
                if (vars.contains(proVar->variable()))
                    *proVars << proVar;
            } else {
                findProVariables(subBlock, vars, proVars);
            }
        }
    }
}

QStringList ProWriter::removeFiles(ProFile *profile, QStringList *lines,
                                   const QDir &proFileDir, const QStringList &filePaths,
                                   const QStringList &vars)
{
    QStringList notChanged = filePaths;

    QList<ProVariable *> proVars;
    findProVariables(profile, vars, &proVars);

    // This is a tad stupid - basically, it can remove only entries which
    // the above code added.
    QStringList relativeFilePaths;
    foreach (const QString &absoluteFilePath, filePaths)
        relativeFilePaths << proFileDir.relativeFilePath(absoluteFilePath);

    // This code expects proVars to be sorted by the variables' appearance in the file.
    int delta = 1;
    foreach (ProVariable *proVar, proVars) {
        if (proVar->variableOperator() != ProVariable::RemoveOperator
            && proVar->variableOperator() != ProVariable::ReplaceOperator) {

            bool first = true;
            int lineNo = proVar->lineNumber() - delta;
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
                        QString fn = line.mid(varCol, colNo - varCol);
                        if (relativeFilePaths.contains(fn)) {
                            notChanged.removeOne(QDir::cleanPath(proFileDir.absoluteFilePath(fn)));
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
    }
    return notChanged;
}
