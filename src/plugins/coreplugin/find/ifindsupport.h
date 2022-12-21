// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    IFindSupport() : QObject(nullptr) {}
    ~IFindSupport() override = default;

    virtual bool supportsReplace() const = 0;
    virtual bool supportsSelectAll() const;
    virtual FindFlags supportedFindFlags() const = 0;
    virtual void resetIncrementalSearch() = 0;
    virtual void clearHighlights() = 0;
    virtual QString currentFindString() const = 0;
    virtual QString completedFindString() const = 0;

    virtual void highlightAll(const QString &, FindFlags) {}
    virtual Result findIncremental(const QString &txt, FindFlags findFlags) = 0;
    virtual Result findStep(const QString &txt, FindFlags findFlags) = 0;
    virtual void replace(const QString &before, const QString &after,
                         FindFlags findFlags);
    virtual bool replaceStep(const QString &before, const QString &after,
        FindFlags findFlags);
    virtual int replaceAll(const QString &before, const QString &after,
        FindFlags findFlags);
    virtual void selectAll(const QString &txt, FindFlags findFlags);

    virtual void defineFindScope(){}
    virtual void clearFindScope(){}

    static void showWrapIndicator(QWidget *parent);

signals:
    void changed();
};

} // namespace Core
