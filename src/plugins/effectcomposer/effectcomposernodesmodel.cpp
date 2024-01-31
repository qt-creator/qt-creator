// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposernodesmodel.h"
#include "effectutils.h"

#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>

namespace EffectComposer {

EffectComposerNodesModel::EffectComposerNodesModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

QHash<int, QByteArray> EffectComposerNodesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CategoryNameRole] = "categoryName";
    roles[CategoryNodesRole] = "categoryNodes";

    return roles;
}

int EffectComposerNodesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_categories.count();
}

QVariant EffectComposerNodesModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_categories.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    return m_categories.at(index.row())->property(roleNames().value(role));
}

void EffectComposerNodesModel::loadModel()
{
    if (m_modelLoaded)
        return;

    auto nodesPath = Utils::FilePath::fromString(EffectUtils::nodesSourcesPath());

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

    m_modelLoaded = true;

    resetModel();
}

void EffectComposerNodesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void EffectComposerNodesModel::updateCanBeAdded(const QStringList &uniforms)
{
    for (const EffectNodesCategory *cat : std::as_const(m_categories)) {
        const QList<EffectNode *> nodes = cat->nodes();
        for (EffectNode *node : nodes) {
            bool match = false;
            for (const QString &uniform : uniforms) {
                match = node->hasUniform(uniform);
                if (match)
                    break;
            }
            node->setCanBeAdded(!match);
        }
    }
}

} // namespace EffectComposer
