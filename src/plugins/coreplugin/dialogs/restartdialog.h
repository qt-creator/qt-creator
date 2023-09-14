// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <QCoreApplication>
#include <QMessageBox>

namespace Core {

class CORE_EXPORT RestartDialog : public QMessageBox
{
public:
    RestartDialog(QWidget *parent, const QString &text);
};

} // namespace Core
