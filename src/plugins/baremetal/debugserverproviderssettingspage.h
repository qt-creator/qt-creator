// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QItemSelectionModel;
class QPushButton;
class QTreeView;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace BareMetal::Internal {

class DebugServerProviderNode;
class DebugServerProvidersSettingsWidget;
class IDebugServerProvider;
class IDebugServerProviderConfigWidget;
class IDebugServerProviderFactory;

// DebugServerProviderModel

class DebugServerProviderModel final
    : public Utils::TreeModel<Utils::TypedTreeItem<DebugServerProviderNode>, DebugServerProviderNode>
{
    Q_OBJECT

public:
    explicit DebugServerProviderModel();

    IDebugServerProvider *provider(const QModelIndex &) const;
    IDebugServerProviderConfigWidget *widget(const QModelIndex &) const;
    DebugServerProviderNode *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForProvider(IDebugServerProvider *provider) const;

    void apply();

    void markForRemoval(IDebugServerProvider *);
    void markForAddition(IDebugServerProvider *);

signals:
    void providerStateChanged();

private:
    void addProvider(IDebugServerProvider *);
    void removeProvider(IDebugServerProvider *);

    DebugServerProviderNode *findNode(const IDebugServerProvider *provider) const;
    DebugServerProviderNode *createNode(IDebugServerProvider *, bool changed);

    QList<IDebugServerProvider *> m_providersToAdd;
    QList<IDebugServerProvider *> m_providersToRemove;
};

// DebugServerProvidersSettingsPage

class DebugServerProvidersSettingsPage final : public Core::IOptionsPage
{
public:
    DebugServerProvidersSettingsPage();
};

} // BareMetal::Internal
