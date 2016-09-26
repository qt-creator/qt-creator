/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    explicit AddNewBackendDialog(QWidget *parent = 0);
    ~AddNewBackendDialog();
    void setupPossibleTypes(const QList<CppTypeData> &types);
    QString importString() const;
    QString type() const;
    bool applied() const;
    bool localDefinition() const;
    bool isSingleton() const;

private:
    void invalidate();

    Ui::AddNewBackendDialog *m_ui;
    QList<CppTypeData> m_typeData;

    bool m_applied = false;
    bool m_isSingleton = false;
};


} // namespace QmlDesigner
