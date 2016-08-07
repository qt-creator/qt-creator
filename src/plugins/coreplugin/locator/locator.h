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

#include "ilocatorfilter.h"
#include "directoryfilter.h"
#include "executefilter.h"
#include "locatorconstants.h"

#include <coreplugin/actionmanager/command.h>

#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

namespace Core {
namespace Internal {

class CorePlugin;
class LocatorWidget;
class OpenDocumentsFilter;
class FileSystemFilter;
class LocatorSettingsPage;
class ExternalToolsFilter;

class Locator : public QObject
{
    Q_OBJECT

public:
    Locator();
    ~Locator();

    void initialize(CorePlugin *corePlugin, const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    bool delayedInitialize();

    QList<ILocatorFilter *> filters();
    QList<ILocatorFilter *> customFilters();
    void setFilters(QList<ILocatorFilter *> f);
    void setCustomFilters(QList<ILocatorFilter *> f);
    int refreshInterval();
    void setRefreshInterval(int interval);

public slots:
    void refresh(QList<ILocatorFilter *> filters = QList<ILocatorFilter *>());
    void saveSettings();
    void openLocator();

private:
    void updatePlaceholderText(Core::Command *command);
    void loadSettings();
    void updateEditorManagerPlaceholderText();

    template <typename S>
    void loadSettingsHelper(S *settings);

    LocatorWidget *m_locatorWidget;
    LocatorSettingsPage *m_settingsPage;

    bool m_settingsInitialized = false;
    QList<ILocatorFilter *> m_filters;
    QList<ILocatorFilter *> m_customFilters;
    int m_refreshInterval;
    QTimer m_refreshTimer;
    OpenDocumentsFilter *m_openDocumentsFilter;
    FileSystemFilter *m_fileSystemFilter;
    ExecuteFilter *m_executeFilter;
    CorePlugin *m_corePlugin = nullptr;
    ExternalToolsFilter *m_externalToolsFilter;
};

template <typename S>
void Locator::loadSettingsHelper(S *settings)
{
    settings->beginGroup(QLatin1String("QuickOpen"));
    m_refreshTimer.setInterval(settings->value(QLatin1String("RefreshInterval"), 60).toInt() * 60000);

    foreach (ILocatorFilter *filter, m_filters) {
        if (settings->contains(filter->id().toString())) {
            const QByteArray state = settings->value(filter->id().toString()).toByteArray();
            if (!state.isEmpty())
                filter->restoreState(state);
        }
    }
    settings->beginGroup(QLatin1String("CustomFilters"));
    QList<ILocatorFilter *> customFilters;
    const QStringList keys = settings->childKeys();
    int count = 0;
    Id baseId(Constants::CUSTOM_FILTER_BASEID);
    foreach (const QString &key, keys) {
        ILocatorFilter *filter = new DirectoryFilter(baseId.withSuffix(++count));
        filter->restoreState(settings->value(key).toByteArray());
        m_filters.append(filter);
        customFilters.append(filter);
    }
    setCustomFilters(customFilters);
    settings->endGroup();
    settings->endGroup();
}

} // namespace Internal
} // namespace Core
