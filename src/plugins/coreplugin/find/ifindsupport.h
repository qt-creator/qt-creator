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

#ifndef IFINDSUPPORT_H
#define IFINDSUPPORT_H

#include "textfindconstants.h"

#include <QObject>
#include <QString>

namespace Core {

class CORE_EXPORT IFindSupport : public QObject
{
    Q_OBJECT

public:
    enum Result { Found, NotFound, NotYetFound };

    IFindSupport() : QObject(0) {}
    virtual ~IFindSupport() {}

    virtual bool supportsReplace() const = 0;
    virtual FindFlags supportedFindFlags() const = 0;
    virtual void resetIncrementalSearch() = 0;
    virtual void clearHighlights() = 0;
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

} // namespace Core

#endif // IFINDSUPPORT_H
