/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CODEPASTERPLUGIN_H
#define CODEPASTERPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace CodePaster {
class CustomFetcher;
class CustomPoster;
struct Settings;
class Protocol;

class CodepasterPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    CodepasterPlugin();
    ~CodepasterPlugin();

    virtual bool initialize(const QStringList &arguments, QString *error_message);
    virtual void extensionsInitialized();
    virtual void shutdown();

public slots:
    void post();
    void fetch();
    void finishPost(const QString &link);
    void finishFetch(const QString &titleDescription,
                     const QString &content,
                     bool error);

private:
    const QSharedPointer<Settings> m_settings;
    QAction *m_postAction;
    QAction *m_fetchAction;
    QList<Protocol*> m_protocols;
    QStringList m_fetchedSnippets;
};

} // namespace CodePaster

#endif // CODEPASTERPLUGIN_H
