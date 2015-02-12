/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CURRENTDOCUMENTFIND_H
#define CURRENTDOCUMENTFIND_H

#include "ifindsupport.h"

#include <QPointer>

namespace Core {
namespace Internal {

class CurrentDocumentFind : public QObject
{
    Q_OBJECT

public:
    CurrentDocumentFind();

    void resetIncrementalSearch();
    void clearHighlights();
    bool supportsReplace() const;
    FindFlags supportedFindFlags() const;
    QString currentFindString() const;
    QString completedFindString() const;

    bool isEnabled() const;
    IFindSupport *candidate() const;
    void highlightAll(const QString &txt, FindFlags findFlags);
    IFindSupport::Result findIncremental(const QString &txt, FindFlags findFlags);
    IFindSupport::Result findStep(const QString &txt, FindFlags findFlags);
    void replace(const QString &before, const QString &after, FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after, FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after, FindFlags findFlags);
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
} // namespace Core

#endif // CURRENTDOCUMENTFIND_H
