/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "ui_settingspage.h"
#include "quickopenconstants.h"

#include <QtCore/QPointer>
#include <QtCore/QHash>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

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
    SettingsPage(Core::ICore *core, QuickOpenPlugin *plugin);
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
    Core::ICore *m_core;
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
