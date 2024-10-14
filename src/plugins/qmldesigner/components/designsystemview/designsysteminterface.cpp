// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "designsysteminterface.h"
#include "collectionmodel.h"

#include <designsystem/dsconstants.h>
#include <QQmlEngine>

namespace QmlDesigner {
DesignSystemInterface::DesignSystemInterface(DSStore *store)
    : m_store(store)
{
    qmlRegisterUncreatableMetaObject(
        QmlDesigner::staticMetaObject, "QmlDesigner.DesignSystem", 1, 0, "GroupType", "");
}

DesignSystemInterface::~DesignSystemInterface() {}

void DesignSystemInterface::loadDesignSystem()
{
    m_models.clear();
    m_store->load();
    emit loadFinished();
}

QAbstractItemModel *DesignSystemInterface::model(const QString &typeName)
{
    if (auto collection = m_store->collection(typeName)) {
        auto itr = m_models.find(typeName);
        if (itr != m_models.end())
            return itr->second.get();

        auto [newItr, success] = m_models.try_emplace(typeName,
                                                      std::make_unique<CollectionModel>(*collection));
        if (success) {
            // Otherwise the model will be deleted by the QML engine.
            QQmlEngine::setObjectOwnership(newItr->second.get(), QQmlEngine::CppOwnership);
            return newItr->second.get();
        }
    }

    return nullptr;
}

QStringList DesignSystemInterface::collections() const
{
    return m_store->collectionNames();
}
} // namespace QmlDesigner
