// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aimodelsmodel.h"

#include "aiassistantconstants.h"
#include "settings/aiprovidersettings.h"

#include <coreplugin/icore.h>

namespace QmlDesigner {

AiModelsModel::AiModelsModel(QObject *parent)
    : QAbstractListModel(parent)
{}

AiModelsModel::~AiModelsModel() {}

int AiModelsModel::rowCount(const QModelIndex &parent) const
{
    return m_data.size();
}

QVariant AiModelsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const int row = index.row();
    const AiModelInfo &info = m_data.at(row);
    switch (static_cast<Roles>(role)) {
    case Roles::SourceIndex:
        return row;
    case Roles::Provider:
        return info.providerName;
    case Roles::ModelId:
    default:
        return info.modelId;
    };
}

QHash<int, QByteArray> AiModelsModel::roleNames() const
{
    static const QHash<int, QByteArray> result = {
        {int(Roles::Provider), "provider"},
        {int(Roles::ModelId), "modelId"},
        {int(Roles::SourceIndex), "sourceIndex"},
    };
    return result;
}

void AiModelsModel::update()
{
    const AiModelInfo oldSelectedInfo = selectedInfo();
    QString selectedProvider;
    QString selectedModelId;
    if (oldSelectedInfo.isValid()) {
        selectedProvider = oldSelectedInfo.providerName;
        selectedModelId = oldSelectedInfo.modelId;
    } else {
        auto settings = Core::ICore::settings();
        settings->beginGroup(Constants::aiAssistantSelectedModelKey);
        selectedProvider = settings->value("providerName").toString();
        selectedModelId = settings->value("modelId").toString();
        settings->endGroup();
    }

    beginResetModel();
    m_data = AiProviderSettings::allValidModels();
    endResetModel();

    if (!selectedProvider.isEmpty() && !selectedModelId.isEmpty()) {
        const int newIdx = findIndex(selectedProvider, selectedModelId);
        if (newIdx > -1)
            setSelectedIndex(newIdx);
    }

    const int numRows = rowCount();
    if ((m_selectedIndex < 0 || m_selectedIndex >= numRows) && numRows > 0)
        setSelectedIndex(0);
}

AiModelInfo AiModelsModel::selectedInfo() const
{
    return at(m_selectedIndex);
}

int AiModelsModel::selectedIndex() const
{
    return m_selectedIndex;
}

void AiModelsModel::setSelectedIndex(int idx)
{
    if (m_selectedIndex == idx)
        return;
    m_selectedIndex = idx;
    emit selectedIndexChanged();

    const AiModelInfo selectedInfo = this->selectedInfo();
    if (!selectedInfo.isValid())
        return;

    auto settings = Core::ICore::settings();
    settings->beginGroup(Constants::aiAssistantSelectedModelKey);
    settings->setValue("providerName", selectedInfo.providerName);
    settings->setValue("modelId", selectedInfo.modelId);
    settings->endGroup();
}

AiModelInfo AiModelsModel::at(int idx) const
{
    if (idx > -1 && idx < m_data.size())
        return m_data.at(idx);
    return {};
}

int AiModelsModel::findIndex(const QString &providerName, const QString &modelId) const
{
    int idx = -1;
    for (const AiModelInfo &modelInfo : std::as_const(m_data)) {
        ++idx;
        if (modelInfo.providerName == providerName && modelInfo.modelId == modelId)
            return idx;
    }
    return -1;
}

} // namespace QmlDesigner
