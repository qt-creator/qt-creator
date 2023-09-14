// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <bindingproperty.h>
#include <modelfwd.h>

#include <QStandardItem>

namespace QmlDesigner {

class BindingModelItem : public QStandardItem
{
public:
    enum UserRoles {
        InternalIdRole = Qt::UserRole + 2,
        TargetNameRole,
        TargetPropertyNameRole,
        SourceNameRole,
        SourcePropertyNameRole
    };

    static QHash<int, QByteArray> roleNames();
    static QStringList headerLabels();

    BindingModelItem(const BindingProperty &property);

    int internalId() const;
    PropertyName targetPropertyName() const;

    void updateProperty(const BindingProperty &property);
};

} // End namespace QmlDesigner.
