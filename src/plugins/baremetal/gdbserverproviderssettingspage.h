/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GDBSERVERPROVIDERSSETTINGSPAGE_H
#define GDBSERVERPROVIDERSSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/treemodel.h>

#include <QPointer>

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

class GdbServerProviderModel : public Utils::TreeModel
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

    QPointer<GdbServerProvidersSettingsWidget> m_configWidget;
};

} // namespace Internal
} // namespace BareMetal

#endif // GDBSERVERPROVIDERSSETTINGSPAGE_H
