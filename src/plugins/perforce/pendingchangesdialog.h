// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

namespace Perforce::Internal {

class PendingChangesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PendingChangesDialog(const QString &data, QWidget *parent = nullptr);
    int changeNumber() const;

private:
    QListWidget *m_listWidget = nullptr;
};

} // Perforce::Internal
