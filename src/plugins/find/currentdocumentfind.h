/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
