/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef IFINDSUPPORT_H
#define IFINDSUPPORT_H

#include "find_global.h"
#include "textfindconstants.h"

#include <QObject>
#include <QString>

namespace Find {

class FIND_EXPORT IFindSupport : public QObject
{
    Q_OBJECT

public:
    enum Result { Found, NotFound, NotYetFound };

    IFindSupport() : QObject(0) {}
    virtual ~IFindSupport() {}

    virtual bool supportsReplace() const = 0;
    virtual FindFlags supportedFindFlags() const = 0;
    virtual void resetIncrementalSearch() = 0;
    virtual void clearResults() = 0;
    virtual QString currentFindString() const = 0;
    virtual QString completedFindString() const = 0;

    virtual void highlightAll(const QString &txt, FindFlags findFlags);
    virtual Result findIncremental(const QString &txt, FindFlags findFlags) = 0;
    virtual Result findStep(const QString &txt, FindFlags findFlags) = 0;
    virtual void replace(const QString &before, const QString &after,
                         FindFlags findFlags);
    virtual bool replaceStep(const QString &before, const QString &after,
        FindFlags findFlags);
    virtual int replaceAll(const QString &before, const QString &after,
        FindFlags findFlags);

    virtual void defineFindScope(){}
    virtual void clearFindScope(){}

    static void showWrapIndicator(QWidget *parent);

signals:
    void changed();
};

inline void IFindSupport::highlightAll(const QString &, FindFlags) {}

} // namespace Find

#endif // IFINDSUPPORT_H
