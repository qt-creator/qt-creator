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

#include "codepasterservice.h"

#include <extensionsystem/iplugin.h>

#include <QStringList>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace CodePaster {
class Settings;
class Protocol;

class CodePasterServiceImpl : public QObject, public CodePaster::Service
{
    Q_OBJECT
    Q_INTERFACES(CodePaster::Service)
public:
    explicit CodePasterServiceImpl(QObject *parent = 0);

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

    void post(PasteSources pasteSources);
    void post(QString data, const QString &mimeType);

private:
    void pasteSnippet();
    void fetch();
    void finishPost(const QString &link);
    void finishFetch(const QString &titleDescription,
                     const QString &content,
                     bool error);

    void fetchUrl();

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
