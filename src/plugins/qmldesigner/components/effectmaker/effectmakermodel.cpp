// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakermodel.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/filepath.h>

namespace QmlDesigner {

EffectMakerModel::EffectMakerModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

QHash<int, QByteArray> EffectMakerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CategoryRole] = "categoryName";
    roles[EffectsRole] = "effectNames";
    return roles;
}

int EffectMakerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_categories.count();
}

QVariant EffectMakerModel::data(const QModelIndex &index, int role) const
{
    // TODO: to be updated

    if (index.row() < 0 || index.row() >= m_categories.count())
        return {};

    const EffectsCategory *category = m_categories[index.row()];
    if (role == CategoryRole)
        return category->name();

    if (role == EffectsRole) {
        QStringList effectsNames;
        const QList<EffectNode *> effects = category->effects();
        for (const EffectNode *effect : effects)
            effectsNames << effect->name();

        return effectsNames;
    }

    return {};
}

// static
Utils::FilePath EffectMakerModel::getQmlEffectsPath()
{
    const ProjectExplorer::Target *target = ProjectExplorer::ProjectTree::currentTarget();
    if (!target) {
        qWarning() << __FUNCTION__ << "No project open";
        return "";
    }

    const QtSupport::QtVersion *baseQtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
    return baseQtVersion->qmlPath().pathAppended("QtQuickEffectMaker/defaultnodes");
}

void EffectMakerModel::loadModel()
{
    const Utils::FilePath effectsPath = getQmlEffectsPath();

    if (!effectsPath.exists()) {
        qWarning() << __FUNCTION__ << "Effects are not found.";
        return;
    }
    QDirIterator itCategories(effectsPath.toString(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (itCategories.hasNext()) {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
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
        EffectsCategory *category = new EffectsCategory(itCategories.fileName(), effects);
        m_categories.push_back(category);
        endInsertRows();
    }
}

void EffectMakerModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void EffectMakerModel::selectEffect(int idx, bool force)
{
    Q_UNUSED(idx)
    Q_UNUSED(force)

    // TODO
}

void EffectMakerModel::applyToSelected(qint64 internalId, bool add)
{
    Q_UNUSED(internalId)
    Q_UNUSED(add)

    // TODO: remove?
}

} // namespace QmlDesigner
