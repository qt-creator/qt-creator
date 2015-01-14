/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FILTERSETTINGSPAGE_H
#define FILTERSETTINGSPAGE_H

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

    QWidget *widget();
    void apply();
    void finish();

signals:
    void filtersChanged();

private slots:
    void updateAttributes(QListWidgetItem *item);
    void updateFilterMap();
    void updateFilterPage();
    void addFilter();
    void removeFilter();

private:
    void updateFilterDescription(const QString &filter);

private:
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

#endif // DOCSETTINGSPAGE_H
