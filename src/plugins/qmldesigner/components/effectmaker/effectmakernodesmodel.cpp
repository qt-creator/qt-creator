// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakernodesmodel.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/filepath.h>

namespace QmlDesigner {

EffectMakerNodesModel::EffectMakerNodesModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

QHash<int, QByteArray> EffectMakerNodesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CategoryNameRole] = "categoryName";
    roles[CategoryNodesRole] = "categoryNodes";

    return roles;
}

int EffectMakerNodesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_categories.count();
}

QVariant EffectMakerNodesModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_categories.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    return m_categories.at(index.row())->property(roleNames().value(role));
}

// static
Utils::FilePath EffectMakerNodesModel::getQmlEffectNodesPath()
{
    const ProjectExplorer::Target *target = ProjectExplorer::ProjectTree::currentTarget();
    if (!target) {
        qWarning() << __FUNCTION__ << "No project open";
        return "";
    }

    const QtSupport::QtVersion *baseQtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
    return baseQtVersion->qmlPath().pathAppended("QtQuickEffectMaker/defaultnodes");
}

void EffectMakerNodesModel::loadModel()
{
    const Utils::FilePath effectsPath = getQmlEffectNodesPath();

    if (!effectsPath.exists()) {
        qWarning() << __FUNCTION__ << "Effects not found.";
        return;
    }

    QDirIterator itCategories(effectsPath.toString(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (itCategories.hasNext()) {
        itCategories.next();

        if (itCategories.fileName() == "images")
            continue;

        QList<EffectNode *> effects = {};
        Utils::FilePath categoryPath = effectsPath.resolvePath(itCategories.fileName());
        QDirIterator itEffects(categoryPath.toString(), QDir::Files | QDir::NoDotAndDotDot);
        while (itEffects.hasNext()) {
            itEffects.next();
            effects.push_back(new EffectNode(QFileInfo(itEffects.fileName()).baseName()));
        }
        EffectNodesCategory *category = new EffectNodesCategory(itCategories.fileName(), effects);
        m_categories.push_back(category);
    }

    resetModel();
}

void EffectMakerNodesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

} // namespace QmlDesigner
