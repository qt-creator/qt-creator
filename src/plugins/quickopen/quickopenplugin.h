/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QUICKOPENPLUGIN_H
#define QUICKOPENPLUGIN_H

#include "ilocatorfilter.h"
#include "directoryfilter.h"

#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QFutureWatcher>

namespace QuickOpen {
namespace Internal {

class LocatorWidget;
class OpenDocumentsFilter;
class FileSystemFilter;
class SettingsPage;
class QuickOpenPlugin;

class QuickOpenPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    QuickOpenPlugin();
    ~QuickOpenPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();

    QList<ILocatorFilter*> filters();
    QList<ILocatorFilter*> customFilters();
    void setFilters(QList<ILocatorFilter*> f);
    void setCustomFilters(QList<ILocatorFilter*> f);
    int refreshInterval();
    void setRefreshInterval(int interval);

public slots:
    void refresh(QList<ILocatorFilter*> filters = QList<ILocatorFilter*>());
    void saveSettings();
    void openQuickOpen();

private slots:
    void startSettingsLoad();
    void settingsLoaded();

private:
    void loadSettings();

    template <typename S>
    void loadSettingsHelper(S *settings);

    LocatorWidget *m_locatorWidget;
    SettingsPage *m_settingsPage;

    QList<ILocatorFilter*> m_filters;
    QList<ILocatorFilter*> m_customFilters;
    int m_refreshInterval;
    QTimer m_refreshTimer;
    OpenDocumentsFilter *m_openDocumentsFilter;
    FileSystemFilter *m_fileSystemFilter;
    QFutureWatcher<void> m_loadWatcher;
};

template <typename S>
void QuickOpenPlugin::loadSettingsHelper(S *settings)
{
    settings->beginGroup("QuickOpen");
    m_refreshTimer.setInterval(settings->value("RefreshInterval", 60).toInt() * 60000);

    foreach (ILocatorFilter *filter, m_filters) {
        if (settings->contains(filter->name())) {
            const QByteArray state = settings->value(filter->name()).toByteArray();
            if (!state.isEmpty())
                filter->restoreState(state);
        }
    }
    settings->beginGroup("CustomFilters");
    QList<ILocatorFilter *> customFilters;
    const QStringList keys = settings->childKeys();
    foreach (const QString &key, keys) {
        ILocatorFilter *filter = new DirectoryFilter;
        filter->restoreState(settings->value(key).toByteArray());
        m_filters.append(filter);
        customFilters.append(filter);
    }
    setCustomFilters(customFilters);
    settings->endGroup();
    settings->endGroup();
}

} // namespace Internal
} // namespace QuickOpen

#endif // QUICKOPENPLUGIN_H
