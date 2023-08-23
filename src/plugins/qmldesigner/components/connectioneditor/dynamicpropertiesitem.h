// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelfwd.h>
#include <QStandardItem>

namespace QmlDesigner {

class AbstractProperty;

class DynamicPropertiesItem : public QStandardItem
{
public:
    enum UserRoles {
        InternalIdRole = Qt::UserRole + 2,
        TargetNameRole,
        PropertyNameRole,
        PropertyTypeRole,
        PropertyValueRole
    };

    static QHash<int, QByteArray> roleNames();
    static QStringList headerLabels();

    DynamicPropertiesItem(const AbstractProperty &property);

    int internalId() const;
    PropertyName propertyName() const;

    void updateProperty(const AbstractProperty &property);
};

} // End namespace QmlDesigner.
