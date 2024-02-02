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
    roles[DescriptionRole] = "uniformDescription";
    roles[ValueRole] = "uniformValue";
    roles[BackendValueRole] = "uniformBackendValue";
    roles[DefaultValueRole] = "uniformDefaultValue";
    roles[MinValueRole] = "uniformMinValue";
    roles[MaxValueRole] = "uniformMaxValue";
    roles[TypeRole] = "uniformType";
    roles[ControlTypeRole] = "uniformControlType";
    roles[UseCustomValueRole] = "uniformUseCustomValue";
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

    if (uniform->type() == Uniform::Type::Sampler) {
        QString updatedValue = value.toString();
        int idx = value.toString().indexOf("file:");

        QString path = idx > 0 ? updatedValue.right(updatedValue.size() - idx - 5) : updatedValue;
        updatedValue = QUrl::fromLocalFile(path).toString();

        uniform->setValue(updatedValue);
        g_propertyData.insert(uniform->name(), updatedValue);
    } else {
        uniform->setValue(value);
        g_propertyData.insert(uniform->name(), value);
    }

    emit dataChanged(index, index, {role});

    return true;
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

QList<Uniform *> EffectComposerUniformsModel::uniforms() const
{
    return m_uniforms;
}

} // namespace EffectComposer

