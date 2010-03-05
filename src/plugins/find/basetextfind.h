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

#include <QtCore/QPointer>
#include <QtGui/QTextCursor>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextEdit;
QT_END_NAMESPACE

namespace Find {

class FIND_EXPORT BaseTextFind : public Find::IFindSupport
{
    Q_OBJECT

public:
    BaseTextFind(QPlainTextEdit *editor);
    BaseTextFind(QTextEdit *editor);

    bool supportsReplace() const;
    IFindSupport::FindFlags supportedFindFlags() const;
    void resetIncrementalSearch();
    void clearResults();
    QString currentFindString() const;
    QString completedFindString() const;

    Result findIncremental(const QString &txt, IFindSupport::FindFlags findFlags);
    Result findStep(const QString &txt, IFindSupport::FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after,
        IFindSupport::FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after,
        IFindSupport::FindFlags findFlags);

    void defineFindScope();
    void clearFindScope();

signals:
    void highlightAll(const QString &txt, Find::IFindSupport::FindFlags findFlags);
    void findScopeChanged(const QTextCursor &);

private:
    bool find(const QString &txt,
              IFindSupport::FindFlags findFlags,
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
