// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

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

private:
    QString getModelQmlText();

    void reset();
    void updateDataStoreProperties();
    void updateSingletonFile();

    ModelPointer m_model;
    QStringList m_collectionNames;
    QString m_dataRelativePath;
};

} // namespace QmlDesigner
