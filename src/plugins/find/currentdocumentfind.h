/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CURRENTDOCUMENTFIND_H
#define CURRENTDOCUMENTFIND_H

#include "ifindfilter.h"

#include <QtCore/QPointer>
#include <QtGui/QWidget>

namespace Find {
namespace Internal {

class CurrentDocumentFind : public QObject
{
    Q_OBJECT

public:
    CurrentDocumentFind();

    void resetIncrementalSearch();
    void clearResults();
    bool supportsReplace() const;
    QString currentFindString() const;
    QString completedFindString() const;

    bool isEnabled() const;
    void highlightAll(const QString &txt, QTextDocument::FindFlags findFlags);
    bool findIncremental(const QString &txt, QTextDocument::FindFlags findFlags);
    bool findStep(const QString &txt, QTextDocument::FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after,
        QTextDocument::FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after,
        QTextDocument::FindFlags findFlags);
    void defineFindScope();
    void clearFindScope();

    void removeConnections();
    bool setFocusToCurrentFindSupport();

    bool eventFilter(QObject *obj, QEvent *event);

signals:
    void changed();

private slots:
    void updateCurrentFindFilter(QWidget *old, QWidget *now);
    void findSupportDestroyed();

private:
    void removeFindSupportConnections();

    QPointer<IFindSupport> m_currentFind;
    QPointer<QWidget> m_currentWidget;
};

} // namespace Internal
} // namespace Find

#endif // CURRENTDOCUMENTFIND_H
