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
#ifndef CodepasterPlugin_H
#define CodepasterPlugin_H

#include "settingspage.h"
#include "fetcher.h"
#include "poster.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icorelistener.h>
#include <projectexplorer/ProjectExplorerInterfaces>
#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

namespace CodePaster {

class CustomFetcher;
class CustomPoster;

class CodepasterPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
                                CodepasterPlugin();
                                ~CodepasterPlugin();

    bool                        initialize(const QStringList &arguments
                                           , QString *error_message);
    void                        extensionsInitialized();

public slots:
    void                        post();
    void                        fetch();

private:

    QAction                     *m_postAction;
    QAction                     *m_fetchAction;
    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;
    SettingsPage                *m_settingsPage;
    CustomFetcher               *m_fetcher;
    CustomPoster                *m_poster;
};

class CustomFetcher : public Fetcher
{
    Q_OBJECT
public:
                                CustomFetcher(const QString &host);

    int                         fetch(int pasteID);
    inline bool                 hadCustomError() { return m_customError; }

    void                        list(QListWidget*);
private slots:
    void                        customRequestFinished(int id, bool error);

private:
    QString                     m_host;
    QListWidget                 *m_listWidget;
    int                         m_id;
    bool                        m_customError;
};

class CustomPoster : public Poster
{
    Q_OBJECT
public:
                                CustomPoster(const QString &host
                                             , bool copyToClipboard = true
                                             , bool displayOutput = true);

private slots:
    void                        customRequestFinished(int id, bool error);
private:
    bool                        m_copy;
    bool                        m_output;
};

} // namespace CodePaster

#endif
