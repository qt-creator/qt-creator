// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace ProjectExplorer {
namespace Internal {

class ParseIssuesDialog : public QDialog
{
    Q_OBJECT
public:
    ParseIssuesDialog(QWidget *parent = nullptr);
    ~ParseIssuesDialog() override;

private:
    void accept() override;

    class Private;
    Private * const d;
};

} // namespace Internal
} // namespace ProjectExplorer
