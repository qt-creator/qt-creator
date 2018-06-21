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

#include "ui_filtersettingspage.h"
#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace Help {
namespace Internal {

class FilterSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    FilterSettingsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

signals:
    void filtersChanged();

private:
    void updateAttributes(QListWidgetItem *item);
    void updateFilterMap();
    void updateFilterPage();
    void addFilter();
    void removeFilter();
    void updateFilterDescription(const QString &filter);
    QString msgFilterLabel(const QString &filter) const;

    Ui::FilterSettingsPage m_ui;
    QPointer<QWidget> m_widget;

    typedef QMap<QString, QStringList> FilterMap;
    FilterMap m_filterMap;
    FilterMap m_filterMapBackup;

    QStringList m_removedFilters;
};

} // namespace Help
} // namespace Internal
