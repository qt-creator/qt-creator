// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

#include "ui_changenumberdialog.h"

namespace Perforce {
namespace Internal {

// Input a change number for pending changes.
class ChangeNumberDialog : public QDialog
{
    Q_OBJECT
public:
    ChangeNumberDialog(QWidget *parent = nullptr);
    int number() const;

private:
    Ui::ChangeNumberDialog m_ui;

};

} // namespace Perforce
} // namespace Internal
