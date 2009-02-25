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


inline void IFindSupport::highlightAll(const QString &, QTextDocument::FindFlags) {}

} // namespace Find

#endif // IFINDSUPPORT_H
