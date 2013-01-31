/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CODEPASTERPLUGIN_H
#define CODEPASTERPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QStringList>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace CodePaster {
class CustomFetcher;
class CustomPoster;
struct Settings;
class Protocol;

class CodePasterService : public QObject
{
    Q_OBJECT
public:
    explicit CodePasterService(QObject *parent = 0);

public slots:
    void postText(const QString &text, const QString &mimeType);
    void postCurrentEditor();
    void postClipboard();
};

class CodepasterPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CodePaster.json")

public:
    CodepasterPlugin();
    ~CodepasterPlugin();

    virtual bool initialize(const QStringList &arguments, QString *errorMessage);
    virtual void extensionsInitialized();
    virtual ShutdownFlag aboutToShutdown();

    static CodepasterPlugin *instance();

public slots:
    void postEditor();
    void postClipboard();
    void fetch();
    void finishPost(const QString &link);
    void finishFetch(const QString &titleDescription,
                     const QString &content,
                     bool error);

    void post(QString data, const QString &mimeType);
    void fetchUrl();
private:

    static CodepasterPlugin *m_instance;
    const QSharedPointer<Settings> m_settings;
    QAction *m_postEditorAction;
    QAction *m_postClipboardAction;
    QAction *m_fetchAction;
    QAction *m_fetchUrlAction;
    QList<Protocol*> m_protocols;
    QStringList m_fetchedSnippets;
    Protocol *m_urlOpen;
};

} // namespace CodePaster

#endif // CODEPASTERPLUGIN_H
