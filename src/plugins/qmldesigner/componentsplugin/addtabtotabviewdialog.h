// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace QmlDesigner {
namespace Ui { class AddTabToTabViewDialog; }

class AddTabToTabViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddTabToTabViewDialog(QWidget *parent = nullptr);
    ~AddTabToTabViewDialog() override;

    static QString create(const QString &tabName, QWidget *parent);

private:
    Ui::AddTabToTabViewDialog *ui;
};

} // namespace QmlDesigner
