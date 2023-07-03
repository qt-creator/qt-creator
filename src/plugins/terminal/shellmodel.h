// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/terminalhooks.h>

#include <QIcon>
#include <QObject>
#include <QString>

#include <memory>

namespace Terminal::Internal {
struct ShellModelPrivate;

struct ShellModelItem
{
    QString name;
    Utils::Terminal::OpenTerminalParameters openParameters;
};

class ShellModel : public QObject
{
public:
    ShellModel(QObject *parent = nullptr);
    ~ShellModel();

    QList<ShellModelItem> local() const;
    QList<ShellModelItem> remote() const;

private:
    std::unique_ptr<ShellModelPrivate> d;
};

} // namespace Terminal::Internal
