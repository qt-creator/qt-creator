/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QItemSelectionModel;
class QPushButton;
class QTreeView;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace BareMetal {
namespace Internal {

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

} // namespace Internal
} // namespace BareMetal
