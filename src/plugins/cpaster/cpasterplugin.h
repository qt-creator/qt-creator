/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPASTERPLUGIN_H
#define CPASTERPLUGIN_H

#include "codepasterservice.h"

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

class CodePasterServiceImpl : public QObject, public CodePaster::Service
{
    Q_OBJECT
    Q_INTERFACES(CodePaster::Service)
public:
    explicit CodePasterServiceImpl(QObject *parent = 0);

public slots:
    void postText(const QString &text, const QString &mimeType) override;
    void postCurrentEditor() override;
    void postClipboard() override;
};

class CodepasterPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CodePaster.json")

public:
    enum PasteSource {
        PasteEditor = 0x1,
        PasteClipboard = 0x2
    };
    Q_DECLARE_FLAGS(PasteSources, PasteSource)

    CodepasterPlugin();
    ~CodepasterPlugin() override;

    bool initialize(const QStringList &arguments, QString *errorMessage) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

    static CodepasterPlugin *instance();

public slots:
    void pasteSnippet();
    void fetch();
    void finishPost(const QString &link);
    void finishFetch(const QString &titleDescription,
                     const QString &content,
                     bool error);

    void post(PasteSources pasteSources);
    void post(QString data, const QString &mimeType);
    void fetchUrl();
private:

    static CodepasterPlugin *m_instance;
    const QSharedPointer<Settings> m_settings;
    QAction *m_postEditorAction = nullptr;
    QAction *m_fetchAction = nullptr;
    QAction *m_fetchUrlAction = nullptr;
    QList<Protocol*> m_protocols;
    QStringList m_fetchedSnippets;
    Protocol *m_urlOpen = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CodepasterPlugin::PasteSources)

} // namespace CodePaster

#endif // CPASTERPLUGIN_H
