// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposernodesmodel.h"

#include "effectutils.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>

namespace EffectComposer {

constexpr QStringView customCatName{u"Custom"};

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
        qWarning() << __FUNCTION__ << "Effects not found in: " << nodesPath;
        return;
    }

    m_categories.clear();
    m_builtInNodeNames.clear();

    QDirIterator itCategories(nodesPath.toUrlishString(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (itCategories.hasNext()) {
        itCategories.next();

        if (itCategories.fileName() == "images" || itCategories.fileName() == "common")
            continue;

        QString catName = itCategories.fileName();

        QList<EffectNode *> effects = {};
        Utils::FilePath categoryPath = nodesPath.resolvePath(itCategories.fileName());
        QDirIterator itEffects(categoryPath.toUrlishString(), {"*.qen"}, QDir::Files);
        while (itEffects.hasNext()) {
            itEffects.next();
            auto node = new EffectNode(itEffects.filePath(), true);
            if (!node->defaultImagesHash().isEmpty())
                m_defaultImagesHash.insert(node->name(), node->defaultImagesHash());
            effects.push_back(node);
            m_builtInNodeNames.append(node->name());
        }

        catName[0] = catName[0].toUpper(); // capitalize first letter
        Utils::sort(effects, &EffectNode::name);

        EffectNodesCategory *category = new EffectNodesCategory(catName, effects);
        m_categories.push_back(category);

        if (catName == customCatName && !effects.isEmpty()) {
            m_builtinCustomNode = effects[0];
            m_customCategory = category;
        }
    }

    std::sort(m_categories.begin(), m_categories.end(),
              [](EffectNodesCategory *a, EffectNodesCategory *b) {
        if (a->name() == customCatName)
            return false;
        if (b->name() == customCatName)
            return true;
        return a->name() < b->name();
    });

    m_modelLoaded = true;

    loadCustomNodes();
}

void EffectComposerNodesModel::loadCustomNodes()
{
    if (!m_customCategory)
        return;

    for (const QString &nodeName : std::as_const(m_customNodeNames))
        m_defaultImagesHash.remove(nodeName);

    m_customNodeNames.clear();

    QList<EffectNode *> effects;

    const Utils::FilePath nodeLibPath = Utils::FilePath::fromString(EffectUtils::nodeLibraryPath());
    const Utils::FilePaths libraryNodes = nodeLibPath.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const Utils::FilePath &nodePath : libraryNodes) {
        const Utils::FilePath qenPath = nodePath.pathAppended(nodePath.fileName() + ".qen");
        auto node = new EffectNode(qenPath.toFSPathString(), false);
        if (!node->defaultImagesHash().isEmpty())
            m_defaultImagesHash.insert(node->name(), node->defaultImagesHash());
        m_customNodeNames.append(node->name());
        effects.push_back(node);
    }

    Utils::sort(effects, &EffectNode::name);

    if (m_customCategory) {
        if (m_builtinCustomNode)
            effects.prepend(m_builtinCustomNode);
        m_customCategory->setNodes(effects);
    }

    resetModel();
}

void EffectComposerNodesModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

bool EffectComposerNodesModel::nodeExists(const QString &name)
{
    return m_customNodeNames.contains(name, Qt::CaseInsensitive)
         || m_builtInNodeNames.contains(name, Qt::CaseInsensitive);
}

bool EffectComposerNodesModel::isBuiltIn(const QString &name)
{
    return m_builtInNodeNames.contains(name, Qt::CaseInsensitive);
}

void EffectComposerNodesModel::updateCanBeAdded(
    const QStringList &uniforms, [[maybe_unused]] const QStringList &nodeNames)
{
    for (const EffectNodesCategory *cat : std::as_const(m_categories)) {
        const QList<EffectNode *> nodes = cat->nodes();
        for (EffectNode *node : nodes) {
            bool match = false;
            for (const QString &uniform : uniforms) {
                if (node->hasUniform(uniform)) {
                    match = true;
                    break;
                }
            }
            node->setCanBeAdded(!match);
        }
    }
}

QHash<QString, QString> EffectComposerNodesModel::defaultImagesForNode(const QString &name) const
{
    return m_defaultImagesHash.value(name);
}

void EffectComposerNodesModel::removeEffectNode(const QString &name)
{
    if (!m_customCategory || name.isEmpty())
        return;

    m_defaultImagesHash.remove(name);
    m_customNodeNames.removeOne(name);
    m_customCategory->removeNode(name);

    QString fileNameBase = EffectUtils::nodeNameToFileName(name);
    Utils::FilePath nodePath = Utils::FilePath::fromString(EffectUtils::nodeLibraryPath())
                                   .pathAppended(fileNameBase);
    if (nodePath.exists())
        nodePath.removeRecursively();

    resetModel();
}

} // namespace EffectComposer
