/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef BASETEXTFIND_H
#define BASETEXTFIND_H

#include "find_global.h"
#include "ifindsupport.h"

#include <QtCore/QPointer>
#include <QtGui/QPlainTextEdit>

namespace Find {

class FIND_EXPORT BaseTextFind : public Find::IFindSupport
{
    Q_OBJECT

public:
    BaseTextFind(QPlainTextEdit *editor);
    BaseTextFind(QTextEdit *editor);

    bool supportsReplace() const;
    void resetIncrementalSearch();
    void clearResults();
    QString currentFindString() const;
    QString completedFindString() const;

    bool findIncremental(const QString &txt, QTextDocument::FindFlags findFlags);
    bool findStep(const QString &txt, QTextDocument::FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after,
        QTextDocument::FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after,
        QTextDocument::FindFlags findFlags);

    void defineFindScope();
    void clearFindScope();

signals:
    void highlightAll(const QString &txt, QTextDocument::FindFlags findFlags);
    void findScopeChanged(const QTextCursor &);

private:
    bool find(const QString &txt,
              QTextDocument::FindFlags findFlags,
              QTextCursor start);

    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor&);
    QTextDocument *document() const;
    bool isReadOnly() const;
    QPointer<QTextEdit> m_editor;
    QPointer<QPlainTextEdit> m_plaineditor;
    QTextCursor m_findScope;
    bool inScope(int startPosition, int endPosition) const;
    int m_incrementalStartPos;
};

} // namespace Find

#endif // BASETEXTFIND_H
