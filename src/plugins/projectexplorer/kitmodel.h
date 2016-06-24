/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "projectexplorer_export.h"

#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Kit;
class KitFactory;
class KitManager;

namespace Internal {

class KitManagerConfigWidget;
class KitNode;

// --------------------------------------------------------------------------
// KitModel:
// --------------------------------------------------------------------------

class KitModel : public Utils::TreeModel<Utils::TreeItem, Utils::TreeItem, KitNode>
{
    Q_OBJECT

public:
    explicit KitModel(QBoxLayout *parentLayout, QObject *parent = nullptr);

    Kit *kit(const QModelIndex &);
    KitNode *kitNode(const QModelIndex &);
    QModelIndex indexOf(Kit *k) const;

    void setDefaultKit(const QModelIndex &index);
    bool isDefaultKit(Kit *k) const;

    ProjectExplorer::Internal::KitManagerConfigWidget *widget(const QModelIndex &);

    void apply();

    void markForRemoval(Kit *k);
    Kit *markForAddition(Kit *baseKit);

signals:
    void kitStateChanged();

private:
    void addKit(ProjectExplorer::Kit *k);
    void updateKit(ProjectExplorer::Kit *k);
    void removeKit(ProjectExplorer::Kit *k);
    void changeDefaultKit();
    void validateKitNames();
    void isAutoDetectedChanged();

    KitNode *findWorkingCopy(Kit *k) const;
    KitNode *createNode(Kit *k);
    void setDefaultNode(KitNode *node);

    Utils::TreeItem *m_autoRoot;
    Utils::TreeItem *m_manualRoot;

    QList<KitNode *> m_toRemoveList;

    QBoxLayout *m_parentLayout;
    KitNode *m_defaultNode = nullptr;

    bool m_keepUnique = true;
};

} // namespace Internal
} // namespace ProjectExplorer
