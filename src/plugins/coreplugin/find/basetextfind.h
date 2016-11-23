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

#pragma once

#include "ifindsupport.h"

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextEdit;
class QTextCursor;
QT_END_NAMESPACE

namespace Core {
struct BaseTextFindPrivate;

class CORE_EXPORT BaseTextFind : public IFindSupport
{
    Q_OBJECT

public:
    explicit BaseTextFind(QPlainTextEdit *editor);
    explicit BaseTextFind(QTextEdit *editor);
    virtual ~BaseTextFind();

    bool supportsReplace() const;
    FindFlags supportedFindFlags() const;
    void resetIncrementalSearch();
    void clearHighlights();
    QString currentFindString() const;
    QString completedFindString() const;

    Result findIncremental(const QString &txt, FindFlags findFlags);
    Result findStep(const QString &txt, FindFlags findFlags);
    void replace(const QString &before, const QString &after, FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after, FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after, FindFlags findFlags);

    void defineFindScope();
    void clearFindScope();

    void highlightAll(const QString &txt, FindFlags findFlags);

signals:
    void highlightAllRequested(const QString &txt, Core::FindFlags findFlags);
    void findScopeChanged(const QTextCursor &start, const QTextCursor &end,
                          int verticalBlockSelectionFirstColumn,
                          int verticalBlockSelectionLastColumn);

private:
    bool find(const QString &txt, FindFlags findFlags, QTextCursor start, bool *wrapped);
    QTextCursor replaceInternal(const QString &before, const QString &after, FindFlags findFlags);

    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor&);
    QTextDocument *document() const;
    bool isReadOnly() const;
    bool inScope(int startPosition, int endPosition) const;
    QTextCursor findOne(const QRegularExpression &expr, const QTextCursor &from, QTextDocument::FindFlags options) const;

    BaseTextFindPrivate *d;
};

} // namespace Core
