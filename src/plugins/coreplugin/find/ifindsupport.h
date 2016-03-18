/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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
