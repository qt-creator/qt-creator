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

#ifndef QUICKOPENPLUGIN_H
#define QUICKOPENPLUGIN_H

#include "iquickopenfilter.h"

#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QFutureWatcher>

namespace QuickOpen {
namespace Internal {

class QuickOpenToolWindow;
class OpenDocumentsFilter;
class FileSystemFilter;
class SettingsPage;

class QuickOpenPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    QuickOpenPlugin();
    ~QuickOpenPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();

    QList<IQuickOpenFilter*> filters();
    QList<IQuickOpenFilter*> customFilters();
    void setFilters(QList<IQuickOpenFilter*> f);
    void setCustomFilters(QList<IQuickOpenFilter*> f);
    int refreshInterval();
    void setRefreshInterval(int interval);

public slots:
    void refresh(QList<IQuickOpenFilter*> filters = QList<IQuickOpenFilter*>());
    void saveSettings();
    void openQuickOpen();

private slots:
    void startSettingsLoad();
    void settingsLoaded();

private:
    void loadSettings();

    QuickOpenToolWindow *m_quickOpenToolWindow;
    SettingsPage *m_settingsPage;

    QList<IQuickOpenFilter*> m_filters;
    QList<IQuickOpenFilter*> m_customFilters;
    int m_refreshInterval;
    QTimer m_refreshTimer;
    OpenDocumentsFilter *m_openDocumentsFilter;
    FileSystemFilter *m_fileSystemFilter;
    QFutureWatcher<void> m_loadWatcher;
};

} // namespace Internal
} // namespace QuickOpen

#endif // QUICKOPENPLUGIN_H
