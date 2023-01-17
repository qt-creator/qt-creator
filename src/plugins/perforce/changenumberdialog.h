// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace Perforce::Internal {

// Input a change number for pending changes.
class ChangeNumberDialog : public QDialog
{
    Q_OBJECT
public:
    ChangeNumberDialog(QWidget *parent = nullptr);
    int number() const;

private:
    QLineEdit *m_lineEdit = nullptr;
};

} // Perforce::Internal
