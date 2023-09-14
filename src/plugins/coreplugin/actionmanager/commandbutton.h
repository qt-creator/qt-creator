// Copyright (C) 2016 Konstantin Tokarev.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/id.h>

#include <QAction>
#include <QPointer>
#include <QString>
#include <QToolButton>

namespace Core {

class Command;

class CORE_EXPORT CommandAction : public QAction
{
    Q_OBJECT

public:
    explicit CommandAction(QWidget *parent = nullptr);
    explicit CommandAction(Utils::Id id, QWidget *parent = nullptr);
    void setCommandId(Utils::Id id);
    QString toolTipBase() const;
    void setToolTipBase(const QString &toolTipBase);

private:
    void updateToolTip();

    QPointer<Command> m_command;
    QString m_toolTipBase;
};

class CORE_EXPORT CommandButton : public QToolButton
{
    Q_OBJECT

public:
    explicit CommandButton(QWidget *parent = nullptr);
    explicit CommandButton(Utils::Id id, QWidget *parent = nullptr);
    void setCommandId(Utils::Id id);
    QString toolTipBase() const;
    void setToolTipBase(const QString &toolTipBase);

private:
    void updateToolTip();

    QPointer<Command> m_command;
    QString m_toolTipBase;
};
}
