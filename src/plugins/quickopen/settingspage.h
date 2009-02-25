/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "ui_settingspage.h"
#include "quickopenconstants.h"

#include <QtCore/QPointer>
#include <QtCore/QHash>

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace QuickOpen {

class IQuickOpenFilter;

namespace Internal {

class QuickOpenPlugin;

class SettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit SettingsPage(QuickOpenPlugin *plugin);
    QString name() const { return tr(Constants::FILTER_OPTIONS_PAGE); }
    QString category() const { return Constants::QUICKOPEN_CATEGORY; }
    QString trCategory() const { return tr(Constants::QUICKOPEN_CATEGORY); }

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

private slots:
    void updateButtonStates();
    void configureFilter(QListWidgetItem *item = 0);
    void addCustomFilter();
    void removeCustomFilter();

private:
    void updateFilterList();
    void saveFilterStates();
    void restoreFilterStates();
    void requestRefresh();

    Ui::SettingsWidget m_ui;
    QuickOpenPlugin *m_plugin;
    QPointer<QWidget> m_page;
    QList<IQuickOpenFilter *> m_filters;
    QList<IQuickOpenFilter *> m_addedFilters;
    QList<IQuickOpenFilter *> m_removedFilters;
    QList<IQuickOpenFilter *> m_customFilters;
    QList<IQuickOpenFilter *> m_refreshFilters;
    QHash<IQuickOpenFilter *, QByteArray> m_filterStates;
};

} // namespace Internal
} // namespace QuickOpen

#endif // SETTINGSPAGE_H
