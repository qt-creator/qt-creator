// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

#include <rewriterview.h>

namespace QmlDesigner {

namespace Ui {
class AddNewBackendDialog;
}

class AddNewBackendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddNewBackendDialog(QWidget *parent = nullptr);
    ~AddNewBackendDialog() override;
    void setupPossibleTypes(const QList<QmlTypeData> &types);
    QString importString() const;
    QString type() const;
    bool applied() const;
    bool localDefinition() const;
    bool isSingleton() const;

private:
    void invalidate();

    Ui::AddNewBackendDialog *m_ui;
    QList<QmlTypeData> m_typeData;

    bool m_applied = false;
    bool m_isSingleton = false;
};


} // namespace QmlDesigner
