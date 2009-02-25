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

#ifndef CODEPASTERPLUGIN_H
#define CODEPASTERPLUGIN_H

#include "settingspage.h"
#include "fetcher.h"
#include "poster.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icorelistener.h>
#include <extensionsystem/iplugin.h>
#include <projectexplorer/projectexplorer.h>

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

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();

public slots:
    void post();
    void fetch();

private:
    QString serverUrl() const;

    QAction *m_postAction;
    QAction *m_fetchAction;
    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;
    SettingsPage  *m_settingsPage;
    CustomFetcher *m_fetcher;
    CustomPoster  *m_poster;
};


class CustomFetcher : public Fetcher
{
    Q_OBJECT

public:
    CustomFetcher(const QString &host);

    int fetch(int pasteID);
    bool hadCustomError() { return m_customError; }

    void list(QListWidget *);

private slots:
    void customRequestFinished(int id, bool error);

private:
    QString m_host;
    QListWidget *m_listWidget;
    int m_id;
    bool m_customError;
};


class CustomPoster : public Poster
{
    Q_OBJECT
public:
    CustomPoster(const QString &host, bool copyToClipboard = true,
                 bool displayOutput = true);

private slots:
    void customRequestFinished(int id, bool error);

private:
    bool m_copy;
    bool m_output;
};

} // namespace CodePaster

#endif // CODEPASTERPLUGIN_H
