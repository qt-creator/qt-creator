/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    virtual ShutdownFlag aboutToShutdown();

public slots:
    void postEditor();
    void postClipboard();
    void fetch();
    void finishPost(const QString &link);
    void finishFetch(const QString &titleDescription,
                     const QString &content,
                     bool error);

private:
    void post(QString data, const QString &mimeType);

    const QSharedPointer<Settings> m_settings;
    QAction *m_postEditorAction;
    QAction *m_postClipboardAction;
    QAction *m_fetchAction;
    QList<Protocol*> m_protocols;
    QStringList m_fetchedSnippets;
};

} // namespace CodePaster

#endif // CODEPASTERPLUGIN_H
