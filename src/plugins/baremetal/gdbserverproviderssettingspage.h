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

class GdbServerProvider;
class GdbServerProviderConfigWidget;
class GdbServerProviderFactory;
class GdbServerProviderNode;
class GdbServerProvidersSettingsWidget;

class GdbServerProviderModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    explicit GdbServerProviderModel(QObject *parent = 0);

    GdbServerProvider *provider(const QModelIndex &) const;
    GdbServerProviderConfigWidget *widget(const QModelIndex &) const;
    GdbServerProviderNode *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForProvider(GdbServerProvider *provider) const;

    void apply();

    void markForRemoval(GdbServerProvider *);
    void markForAddition(GdbServerProvider *);

signals:
    void providerStateChanged();

private:
    void addProvider(GdbServerProvider *);
    void removeProvider(GdbServerProvider *);

    GdbServerProviderNode *findNode(const GdbServerProvider *provider) const;
    GdbServerProviderNode *createNode(GdbServerProvider *, bool changed);

    QList<GdbServerProvider *> m_providersToAdd;
    QList<GdbServerProvider *> m_providersToRemove;
};

class GdbServerProvidersSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit GdbServerProvidersSettingsPage(QObject *parent = 0);

private:
    QWidget *widget();
    void apply();
    void finish();

    GdbServerProvidersSettingsWidget *m_configWidget = 0;
};

} // namespace Internal
} // namespace BareMetal
