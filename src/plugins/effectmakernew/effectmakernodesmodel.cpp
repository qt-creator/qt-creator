// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakernodesmodel.h"

#include <utils/hostosinfo.h>

#include <QCoreApplication>

namespace EffectMaker {

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

void EffectMakerNodesModel::findNodesPath()
{
    if (m_nodesPath.exists() || m_probeNodesDir)
        return;

    QDir nodesDir;

    if (!qEnvironmentVariable("EFFECT_MAKER_NODES_PATH").isEmpty())
        nodesDir.setPath(qEnvironmentVariable("EFFECT_MAKER_NODES_PATH"));
    else if (Utils::HostOsInfo::isMacHost())
        nodesDir.setPath(QCoreApplication::applicationDirPath() + "/../Resources/effect_maker_nodes");

    // search for nodesDir from exec dir and up
    if (nodesDir.dirName() == ".") {
        m_probeNodesDir = true; // probe only once
        nodesDir.setPath(QCoreApplication::applicationDirPath());
        while (!nodesDir.cd("effect_maker_nodes") && nodesDir.cdUp())
            ; // do nothing

        if (nodesDir.dirName() != "effect_maker_nodes") // bundlePathDir not found
            return;
    }

    m_nodesPath = Utils::FilePath::fromString(nodesDir.path());
}

void EffectMakerNodesModel::loadModel()
{
    findNodesPath();

    if (!m_nodesPath.exists()) {
        qWarning() << __FUNCTION__ << "Effects not found.";
        return;
    }

    m_categories = {};

    QDirIterator itCategories(m_nodesPath.toString(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (itCategories.hasNext()) {
        itCategories.next();

        if (itCategories.fileName() == "images" || itCategories.fileName() == "common")
            continue;

        QString catName = itCategories.fileName();

        QList<EffectNode *> effects = {};
        Utils::FilePath categoryPath = m_nodesPath.resolvePath(itCategories.fileName());
        QDirIterator itEffects(categoryPath.toString(), {"*.qen"}, QDir::Files);
        while (itEffects.hasNext()) {
            itEffects.next();
            effects.push_back(new EffectNode(itEffects.filePath()));
        }

        catName[0] = catName[0].toUpper(); // capitalize first letter
        EffectNodesCategory *category = new EffectNodesCategory(catName, effects);
        m_categories.push_back(category);
    }

    resetModel();
}

void EffectMakerNodesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

} // namespace EffectMaker

