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

#ifndef DIFFER_H
#define DIFFER_H

#include "diffeditor_global.h"
#include <QString>

QT_BEGIN_NAMESPACE
template <class K, class T>
class QMap;
QT_END_NAMESPACE

namespace DiffEditor {

class DIFFEDITOR_EXPORT Diff
{
public:
    enum Command {
        Delete,
        Insert,
        Equal
    };
    Command command;
    QString text;
    Diff(Command com, const QString &txt);
    Diff();
    bool operator==(const Diff &other) const;
    bool operator!=(const Diff &other) const;
    QString toString() const;
    static QString commandString(Command com);
};

class DIFFEDITOR_EXPORT Differ
{
public:
    enum DiffMode
    {
        CharMode,
        WordMode,
        LineMode
    };
    Differ();
    QList<Diff> diff(const QString &text1, const QString &text2);
    void setDiffMode(DiffMode mode);
    bool diffMode() const;
    static QList<Diff> merge(const QList<Diff> &diffList);
    static QList<Diff> cleanupSemantics(const QList<Diff> &diffList);
    static QList<Diff> cleanupSemanticsLossless(const QList<Diff> &diffList);

private:
    QList<Diff> preprocess1AndDiff(const QString &text1, const QString &text2);
    QList<Diff> preprocess2AndDiff(const QString &text1, const QString &text2);
    QList<Diff> diffMyers(const QString &text1, const QString &text2);
    QList<Diff> diffMyersSplit(const QString &text1, int x,
                               const QString &text2, int y);
    QList<Diff> diffNonCharMode(const QString text1, const QString text2);
    QStringList encode(const QString &text1,
                       const QString &text2,
                       QString *encodedText1,
                       QString *encodedText2);
    QString encode(const QString &text,
                   QStringList *lines,
                   QMap<QString, int> *lineToCode);
    int findSubtextEnd(const QString &text,
                       int subTextStart);
    DiffMode m_diffMode;
    DiffMode m_currentDiffMode;
};

} // namespace DiffEditor

#endif // DIFF_H
