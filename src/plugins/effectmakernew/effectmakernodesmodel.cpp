// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakernodesmodel.h"

#include <coreplugin/icore.h>

#include <utils/filepath.h>
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

QString EffectMakerNodesModel::nodesSourcesPath() const
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/effectMakerNodes";
#endif
    return Core::ICore::resourcePath("qmldesigner/effectMakerNodes").toString();
}

void EffectMakerNodesModel::loadModel()
{
    auto nodesPath = Utils::FilePath::fromString(nodesSourcesPath());

    if (!nodesPath.exists()) {
        qWarning() << __FUNCTION__ << "Effects not found.";
        return;
    }

    m_categories = {};

    QDirIterator itCategories(nodesPath.toString(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (itCategories.hasNext()) {
        itCategories.next();

        if (itCategories.fileName() == "images" || itCategories.fileName() == "common")
            continue;

        QString catName = itCategories.fileName();

        QList<EffectNode *> effects = {};
        Utils::FilePath categoryPath = nodesPath.resolvePath(itCategories.fileName());
        QDirIterator itEffects(categoryPath.toString(), {"*.qen"}, QDir::Files);
        while (itEffects.hasNext()) {
            itEffects.next();
            effects.push_back(new EffectNode(itEffects.filePath()));
        }

        catName[0] = catName[0].toUpper(); // capitalize first letter
        EffectNodesCategory *category = new EffectNodesCategory(catName, effects);
        m_categories.push_back(category);
    }

    std::sort(m_categories.begin(), m_categories.end(),
              [](EffectNodesCategory *a, EffectNodesCategory *b) {
        return a->name() < b->name();
    });

    resetModel();
}

void EffectMakerNodesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

} // namespace EffectMaker
