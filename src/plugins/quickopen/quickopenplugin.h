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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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

    QList<IQuickOpenFilter*> filter();
    QList<IQuickOpenFilter*> customFilter();
    void setFilter(QList<IQuickOpenFilter*> f);
    void setCustomFilter(QList<IQuickOpenFilter*> f);
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

    QList<IQuickOpenFilter*> m_filter;
    QList<IQuickOpenFilter*> m_customFilter;
    int m_refreshInterval;
    QTimer m_refreshTimer;
    OpenDocumentsFilter *m_openDocumentsFilter;
    FileSystemFilter *m_fileSystemFilter;
    QFutureWatcher<void> m_loadWatcher;
};

} // namespace Internal
} // namespace QuickOpen

#endif // QUICKOPENPLUGIN_H
