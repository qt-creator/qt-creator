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

#include "ui_locatorsettingspage.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/treemodel.h>

#include <QHash>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Core {

class ILocatorFilter;

namespace Internal {

class Locator;

class LocatorSettingsPage : public IOptionsPage
{
    Q_OBJECT

public:
    explicit LocatorSettingsPage(Locator *plugin);

    QWidget *widget();
    void apply();
    void finish();

private:
    void updateButtonStates();
    void configureFilter(const QModelIndex &proxyIndex);
    void addCustomFilter();
    void removeCustomFilter();
    void initializeModel();
    void saveFilterStates();
    void restoreFilterStates();
    void requestRefresh();
    void setFilter(const QString &text);

    Ui::LocatorSettingsWidget m_ui;
    Locator *m_plugin;
    QPointer<QWidget> m_widget;
    Utils::TreeModel<> *m_model;
    QSortFilterProxyModel *m_proxyModel;
    Utils::TreeItem *m_customFilterRoot;
    QList<ILocatorFilter *> m_filters;
    QList<ILocatorFilter *> m_addedFilters;
    QList<ILocatorFilter *> m_removedFilters;
    QList<ILocatorFilter *> m_customFilters;
    QList<ILocatorFilter *> m_refreshFilters;
    QHash<ILocatorFilter *, QByteArray> m_filterStates;
};

} // namespace Internal
} // namespace Core
