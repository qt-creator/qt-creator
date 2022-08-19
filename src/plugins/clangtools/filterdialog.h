// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace ClangTools {
namespace Internal {

namespace Ui { class FilterDialog; }

class FilterChecksModel;

class Check {
public:
    QString name;
    QString displayName;
    int count = 0;
    bool isShown = false;
    bool hasFixit = false;
};
using Checks = QList<Check>;

class FilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDialog(const Checks &selectedChecks, QWidget *parent = nullptr);
    ~FilterDialog();

    QSet<QString> selectedChecks() const;

private:
    Ui::FilterDialog *m_ui;
    FilterChecksModel *m_model;
};

} // namespace Internal
} // namespace ClangTools
