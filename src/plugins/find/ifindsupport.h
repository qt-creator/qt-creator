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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#ifndef IFINDSUPPORT_H
#define IFINDSUPPORT_H

#include "find_global.h"
#include <QtGui/QTextDocument>

namespace Find {

class FIND_EXPORT IFindSupport : public QObject
{
    Q_OBJECT

public:

    IFindSupport() : QObject(0) {}
    virtual ~IFindSupport() {}

    virtual bool supportsReplace() const = 0;
    virtual void resetIncrementalSearch() = 0;
    virtual void clearResults() = 0;
    virtual QString currentFindString() const = 0;
    virtual QString completedFindString() const = 0;

    virtual void highlightAll(const QString &txt, QTextDocument::FindFlags findFlags);
    virtual bool findIncremental(const QString &txt, QTextDocument::FindFlags findFlags) = 0;
    virtual bool findStep(const QString &txt, QTextDocument::FindFlags findFlags) = 0;
    virtual bool replaceStep(const QString &before, const QString &after,
        QTextDocument::FindFlags findFlags) = 0;
    virtual int replaceAll(const QString &before, const QString &after,
        QTextDocument::FindFlags findFlags) = 0;

    virtual void defineFindScope(){}
    virtual void clearFindScope(){}

signals:
    void changed();
};


inline void IFindSupport::highlightAll(const QString &, QTextDocument::FindFlags){}

} // namespace Find

#endif // IFINDSUPPORT_H
