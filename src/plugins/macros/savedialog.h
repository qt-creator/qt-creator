// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace Macros::Internal {

class SaveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveDialog(QWidget *parent = nullptr);
    ~SaveDialog() override;

    QString name() const;
    QString description() const;

private:
    QLineEdit *m_name;
    QLineEdit *m_description;
};

} // Macros::Internal
