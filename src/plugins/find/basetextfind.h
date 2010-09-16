/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef BASETEXTFIND_H
#define BASETEXTFIND_H

#include "find_global.h"
#include "ifindsupport.h"

#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextEdit;
class QTextCursor;
QT_END_NAMESPACE

namespace Find {
struct BaseTextFindPrivate;

class FIND_EXPORT BaseTextFind : public Find::IFindSupport
{
    Q_OBJECT

public:
    explicit BaseTextFind(QPlainTextEdit *editor);
    explicit BaseTextFind(QTextEdit *editor);
    virtual ~BaseTextFind();

    bool supportsReplace() const;
    Find::FindFlags supportedFindFlags() const;
    void resetIncrementalSearch();
    void clearResults();
    QString currentFindString() const;
    QString completedFindString() const;

    Result findIncremental(const QString &txt, Find::FindFlags findFlags);
    Result findStep(const QString &txt, Find::FindFlags findFlags);
    void replace(const QString &before, const QString &after,
        Find::FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after,
        Find::FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after,
        Find::FindFlags findFlags);

    void defineFindScope();
    void clearFindScope();

signals:
    void highlightAll(const QString &txt, Find::FindFlags findFlags);
    void findScopeChanged(const QTextCursor &start, const QTextCursor &end,
                          int verticalBlockSelectionFirstColumn,
                          int verticalBlockSelectionLastColumn);

private:
    bool find(const QString &txt,
              Find::FindFlags findFlags,
              QTextCursor start);
    QTextCursor replaceInternal(const QString &before, const QString &after,
                                Find::FindFlags findFlags);

    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor&);
    QTextDocument *document() const;
    bool isReadOnly() const;
    bool inScope(int startPosition, int endPosition) const;
    QTextCursor findOne(const QRegExp &expr, const QTextCursor &from, QTextDocument::FindFlags options) const;

    QScopedPointer<BaseTextFindPrivate> d;
};

} // namespace Find

#endif // BASETEXTFIND_H
