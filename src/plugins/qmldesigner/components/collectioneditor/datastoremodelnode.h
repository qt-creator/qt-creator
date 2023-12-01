// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

#include <QMap>

namespace QmlDesigner {

class Model;

class DataStoreModelNode
{
public:
    DataStoreModelNode();

    void reloadModel();
    QStringList collectionNames() const;

    Model *model() const;
    ModelNode modelNode() const;

    void setCollectionNames(const QStringList &newCollectionNames);
    void renameCollection(const QString &oldName, const QString &newName);
    void removeCollection(const QString &collectionName);

    void assignCollectionToNode(AbstractView *view,
                                const ModelNode &targetNode,
                                const QString &collectionName);

private:
    QString getModelQmlText();

    void reset();
    void updateDataStoreProperties();
    void updateSingletonFile();
    void update();
    PropertyName getUniquePropertyName(const QString &collectionName);

    ModelPointer m_model;
    QMap<QString, PropertyName> m_collectionPropertyNames;
    QString m_dataRelativePath;
};

} // namespace QmlDesigner
