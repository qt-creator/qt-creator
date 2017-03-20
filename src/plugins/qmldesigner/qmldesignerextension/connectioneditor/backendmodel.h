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
#include <QStandardItemModel>

#include "rewriterview.h"

namespace QmlDesigner {

namespace Internal {

class ConnectionView;

class BackendModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum ColumnRoles {
        TypeNameColumn = 0,
        PropertyNameColumn = 1,
        IsSingletonColumn = 2,
        IsLocalColumn = 3,
    };

    BackendModel(ConnectionView *parent);

    ConnectionView *connectionView() const;

    void resetModel();

    QStringList possibleCppTypes() const;
    CppTypeData cppTypeDataForType(const QString &typeName) const;

    void deletePropertyByRow(int rowNumber);

    void addNewBackend();

protected:
    void updatePropertyName(int rowNumber);

private:
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex& bottomRight);

private:
    ConnectionView *m_connectionView;
    bool m_lock = false;
};

} // namespace Internal

} // namespace QmlDesigner
