// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectnodescategory.h"

#include <QStandardItemModel>

namespace EffectComposer {

class EffectNode;

class EffectComposerNodesModel : public QAbstractListModel
{
    Q_OBJECT

    enum Roles {
        CategoryNameRole = Qt::UserRole + 1,
        CategoryNodesRole
    };

public:
    EffectComposerNodesModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void loadModel();
    void loadCustomNodes();
    void resetModel();

    bool nodeExists(const QString &name);
    bool isBuiltIn(const QString &name);

    QList<EffectNodesCategory *> categories() const { return  m_categories; }

    void updateCanBeAdded(const QStringList &uniforms, const QStringList &nodeNames);

    QHash<QString, QString> defaultImagesForNode(const QString &name) const;

    void removeEffectNode(const QString &name);

private:
    QString nodesSourcesPath() const;

    QList<EffectNodesCategory *> m_categories;
    bool m_probeNodesDir = false;
    bool m_modelLoaded = false;
    QHash<QString, QHash<QString, QString>> m_defaultImagesHash;
    QStringList m_builtInNodeNames;
    QStringList m_customNodeNames;
    EffectNode *m_builtinCustomNode = nullptr;
    EffectNodesCategory *m_customCategory = nullptr;
};

} // namespace EffectComposer

