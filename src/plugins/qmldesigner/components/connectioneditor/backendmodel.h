// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once
#include <QStandardItemModel>

#include "rewriterview.h"

namespace QmlDesigner {

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
    QmlTypeData cppTypeDataForType(const QString &typeName) const;

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

} // namespace QmlDesigner
