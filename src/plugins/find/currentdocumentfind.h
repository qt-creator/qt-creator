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

#ifndef CURRENTDOCUMENTFIND_H
#define CURRENTDOCUMENTFIND_H

#include "ifindsupport.h"

#include <QPointer>

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
    Find::FindFlags supportedFindFlags() const;
    QString currentFindString() const;
    QString completedFindString() const;

    bool isEnabled() const;
    bool candidateIsEnabled() const;
    void highlightAll(const QString &txt, Find::FindFlags findFlags);
    IFindSupport::Result findIncremental(const QString &txt, Find::FindFlags findFlags);
    IFindSupport::Result findStep(const QString &txt, Find::FindFlags findFlags);
    void replace(const QString &before, const QString &after,
        Find::FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after,
        Find::FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after,
        Find::FindFlags findFlags);
    void defineFindScope();
    void clearFindScope();
    void acceptCandidate();

    void removeConnections();
    bool setFocusToCurrentFindSupport();

    bool eventFilter(QObject *obj, QEvent *event);

signals:
    void changed();
    void candidateChanged();

private slots:
    void updateCandidateFindFilter(QWidget *old, QWidget *now);
    void clearFindSupport();
    void aggregationChanged();
    void candidateAggregationChanged();

private:
    void removeFindSupportConnections();

    QPointer<IFindSupport> m_currentFind;
    QPointer<QWidget> m_currentWidget;
    QPointer<IFindSupport> m_candidateFind;
    QPointer<QWidget> m_candidateWidget;
};

} // namespace Internal
} // namespace Find

#endif // CURRENTDOCUMENTFIND_H
