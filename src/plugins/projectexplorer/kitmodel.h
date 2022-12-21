// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Kit;
class KitFactory;
class KitManager;

namespace Internal {

class KitManagerConfigWidget;
class KitNode;

// --------------------------------------------------------------------------
// KitModel:
// --------------------------------------------------------------------------

class KitModel : public Utils::TreeModel<Utils::TreeItem, Utils::TreeItem, KitNode>
{
    Q_OBJECT

public:
    explicit KitModel(QBoxLayout *parentLayout, QObject *parent = nullptr);

    Kit *kit(const QModelIndex &);
    KitNode *kitNode(const QModelIndex &);
    QModelIndex indexOf(Kit *k) const;

    void setDefaultKit(const QModelIndex &index);
    bool isDefaultKit(Kit *k) const;

    ProjectExplorer::Internal::KitManagerConfigWidget *widget(const QModelIndex &);

    void apply();

    void markForRemoval(Kit *k);
    Kit *markForAddition(Kit *baseKit);

    void updateVisibility();

    QString newKitName(const QString &sourceName) const;

signals:
    void kitStateChanged();

private:
    void addKit(ProjectExplorer::Kit *k);
    void updateKit(ProjectExplorer::Kit *k);
    void removeKit(ProjectExplorer::Kit *k);
    void changeDefaultKit();
    void validateKitNames();

    KitNode *findWorkingCopy(Kit *k) const;
    KitNode *createNode(Kit *k);
    void setDefaultNode(KitNode *node);

    Utils::TreeItem *m_autoRoot;
    Utils::TreeItem *m_manualRoot;

    QList<KitNode *> m_toRemoveList;

    QBoxLayout *m_parentLayout;
    KitNode *m_defaultNode = nullptr;

    bool m_keepUnique = true;
};

} // namespace Internal
} // namespace ProjectExplorer
