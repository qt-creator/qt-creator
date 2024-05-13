// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

#include <QMap>

namespace Utils {
class FilePath;
}

namespace QmlDesigner {

class Model;

class DataStoreModelNode
{
public:
    using CollectionColumnFinder = std::function<bool(const QString &collectionName,
                                                      const QString &columnName)>;
    using FirstColumnProvider = std::function<QString(const QString &collectionName)>;

    DataStoreModelNode();

    void reloadModel(const Utils::FilePath &projectModulePath);
    QStringList collectionNames() const;

    Model *model() const;
    ModelNode modelNode() const;
    Utils::FilePath jsonFilePath() const;

    void setCollectionNames(const QStringList &newCollectionNames);
    void addCollection(const QString &collectionName);
    void renameCollection(const QString &oldName, const QString &newName);
    void removeCollections(const QStringList &collectionNames);

    void assignCollectionToNode(AbstractView *view,
                                const ModelNode &targetNode,
                                const QString &collectionName,
                                CollectionColumnFinder collectionHasColumn,
                                FirstColumnProvider firstColumnProvider);

private:
    QString getModelQmlText();

    void reset();
    void preloadFile();
    void updateDataStoreProperties();
    void updateSingletonFile();
    void update();
    void addCollectionNameToTheModel(const QString &collectionName,
                                     const PropertyName &dataStorePropertyName);
    Utils::FilePath dataStoreQmlFilePath() const;

    PropertyName getUniquePropertyName(const QString &collectionName);

    ModelPointer m_model;
    QMap<QString, PropertyName> m_collectionPropertyNames;
    QString m_dataRelativePath;
};

} // namespace QmlDesigner
