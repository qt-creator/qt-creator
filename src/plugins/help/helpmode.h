// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/imode.h>

#include <QString>
#include <QIcon>

namespace Help {
namespace Internal {

class HelpMode : public Core::IMode
{
public:
    explicit HelpMode(QObject *parent = nullptr);
};

} // namespace Internal
} // namespace Help
