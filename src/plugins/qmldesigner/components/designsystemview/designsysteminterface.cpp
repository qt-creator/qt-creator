// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "designsysteminterface.h"
#include "collectionmodel.h"

#include <designsystem/dsconstants.h>
#include <designsystem/dsthememanager.h>
#include <qmldesignerutils/memory.h>

#include <QQmlEngine>

namespace QmlDesigner {
DesignSystemInterface::DesignSystemInterface()
{
    qmlRegisterUncreatableMetaObject(
        QmlDesigner::staticMetaObject, "QmlDesigner.DesignSystem", 1, 0, "GroupType", "");
    qmlRegisterUncreatableType<CollectionModel>("QmlDesigner.DesignSystem", 1, 0, "CollectionModel", "");
}

DesignSystemInterface::~DesignSystemInterface() {}

void DesignSystemInterface::loadDesignSystem()
{
    QTC_ASSERT(m_store, return);

    m_models.clear();

    if (auto err = m_store->load())
        qDebug() << err;

    emit collectionsChanged();
}

CollectionModel *DesignSystemInterface::model(const QString &typeName)
{
    QTC_ASSERT(m_store, return nullptr);

    if (auto collection = m_store->collection(typeName))
        return createModel(typeName, collection);

    return nullptr;
}

QString DesignSystemInterface::generateCollectionName(const QString &hint) const
{
    QTC_ASSERT(m_store, return {});
    return m_store->uniqueCollectionName(hint);
}

void DesignSystemInterface::addCollection(const QString &name)
{
    QTC_ASSERT(m_store, return);

    if (m_store->addCollection(name))
        emit collectionsChanged();
}

void DesignSystemInterface::removeCollection(const QString &name)
{
    QTC_ASSERT(m_store, return);

    if (m_store->collection(name)) {
        m_models.erase(name);
        m_store->removeCollection(name);
        emit collectionsChanged();
    }
}

void DesignSystemInterface::renameCollection(const QString &oldName, const QString &newName)
{
    QTC_ASSERT(m_store, return);

    if (m_store->renameCollection(oldName, newName))
        emit collectionsChanged();
}

ThemeProperty DesignSystemInterface::createThemeProperty(const QString &name,
                                                         const QVariant &value,
                                                         bool isBinding) const
{
    return {name.toUtf8(), value, isBinding};
}

QStringList DesignSystemInterface::collections() const
{
    QTC_ASSERT(m_store, return {});

    return m_store->collectionNames();
}

void DesignSystemInterface::setDSStore(DSStore *store)
{
    m_store = store;
}

CollectionModel *DesignSystemInterface::createModel(const QString &typeName, DSThemeManager *collection)
{
    auto [iterator, inserted] = m_models.try_emplace(typeName, collection, m_store);
    if (inserted) {
        // Otherwise the model will be deleted by the QML engine.
        QQmlEngine::setObjectOwnership(&iterator->second, QQmlEngine::CppOwnership);
    }

    return &iterator->second;
}
} // namespace QmlDesigner
