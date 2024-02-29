// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT IWelcomePage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(int priority READ priority CONSTANT)

public:
    IWelcomePage();
    ~IWelcomePage() override;

    virtual QString title() const = 0;
    virtual int priority() const { return 0; }
    virtual Utils::Id id() const = 0;
    virtual QWidget *createWidget() const = 0;

    static const QList<IWelcomePage *> allWelcomePages();
};

} // Core
