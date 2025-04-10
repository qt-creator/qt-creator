// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposeruniformsmodel.h"

#include "propertyhandler.h"
#include "uniform.h"

#include <utils/qtcassert.h>

namespace EffectComposer {

EffectComposerUniformsModel::EffectComposerUniformsModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

QHash<int, QByteArray> EffectComposerUniformsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "uniformName";
    roles[DisplayNameRole] = "uniformDisplayName";
    roles[DescriptionRole] = "uniformDescription";
    roles[ValueRole] = "uniformValue";
    roles[BackendValueRole] = "uniformBackendValue";
    roles[DefaultValueRole] = "uniformDefaultValue";
    roles[MinValueRole] = "uniformMinValue";
    roles[MaxValueRole] = "uniformMaxValue";
    roles[TypeRole] = "uniformType";
    roles[ControlTypeRole] = "uniformControlType";
    roles[UseCustomValueRole] = "uniformUseCustomValue";
    roles[UserAdded] = "uniformUserAdded";
    roles[IsInUse] = "uniformIsInUse";
    return roles;
}

int EffectComposerUniformsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_uniforms.size();
}

QVariant EffectComposerUniformsModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_uniforms.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    return m_uniforms.at(index.row())->property(roleNames().value(role));
}

bool EffectComposerUniformsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !roleNames().contains(role))
        return false;

    auto uniform = m_uniforms.at(index.row());

    if (role == IsInUse) {
        uniform->setIsInUse(value.toBool());
    } else if (uniform->type() == Uniform::Type::Sampler) {
        QString updatedValue = value.toString();
        int idx = value.toString().indexOf("file:");

        QString path = idx > 0 ? updatedValue.right(updatedValue.size() - idx - 5) : updatedValue;

        if (idx == -1)
            updatedValue = QUrl::fromLocalFile(path).toString();

        uniform->setValue(updatedValue);
        g_propertyData()->insert(uniform->name(), updatedValue);
    } else {
        uniform->setValue(value);
        g_propertyData()->insert(uniform->name(), value);
    }

    emit dataChanged(index, index, {role});

    return true;
}

bool EffectComposerUniformsModel::resetData(int row)
{
    QModelIndex idx = index(row, 0);
    QTC_ASSERT(idx.isValid(), return false);

    return setData(idx, idx.data(DefaultValueRole), ValueRole);
}

bool EffectComposerUniformsModel::remove(int row)
{
    QModelIndex idx = index(row, 0);
    QTC_ASSERT(idx.isValid(), return false);

    beginRemoveRows({}, row, row);
    m_uniforms.removeAt(row);
    endRemoveRows();

    return true;
}

QStringList EffectComposerUniformsModel::displayNames() const
{
    QStringList displayNames;
    for (Uniform *u : std::as_const(m_uniforms))
        displayNames.append(u->displayName());
    return displayNames;
}

QStringList EffectComposerUniformsModel::uniformNames() const
{
    return Utils::transform(m_uniforms, &Uniform::name);
}

void EffectComposerUniformsModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void EffectComposerUniformsModel::addUniform(Uniform *uniform)
{
    beginInsertRows({}, m_uniforms.size(), m_uniforms.size());
    m_uniforms.append(uniform);
    endInsertRows();
}

void EffectComposerUniformsModel::updateUniform(int uniformIndex, Uniform *uniform)
{
    QTC_ASSERT(uniformIndex < m_uniforms.size() && uniformIndex >= 0, return);

    Uniform *oldUniform = m_uniforms.at(uniformIndex);

    // Do full row remove + insert to make sure UI updates the type controls properly
    beginRemoveRows({}, uniformIndex, uniformIndex);
    m_uniforms.removeAt(uniformIndex);
    endRemoveRows();

    beginInsertRows({}, uniformIndex, uniformIndex);
    m_uniforms.insert(uniformIndex, uniform);
    endInsertRows();

    const QString &oldName = oldUniform->name();
    const QString &newName = uniform->name();
    if (oldName != newName)
        emit uniformRenamed(oldName, newName);

    delete oldUniform;
}

QList<Uniform *> EffectComposerUniformsModel::uniforms() const
{
    return m_uniforms;
}

} // namespace EffectComposer

