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

#include "cpasterplugin.h"

#include "splitter.h"
#include "pasteview.h"
#include "codepasterprotocol.h"
#include "kdepasteprotocol.h"
#include "pastebindotcomprotocol.h"
#include "pastebindotcaprotocol.h"
#include "fileshareprotocol.h"
#include "pasteselectdialog.h"
#include "settingspage.h"
#include "settings.h"
#include "urlopenprotocol.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <texteditor/itexteditor.h>

#include <QtPlugin>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QInputDialog>
#include <QUrl>

using namespace Core;
using namespace TextEditor;

namespace CodePaster {

/*!
   \class CodePaster::CodePasterService
   \brief Service registered with PluginManager providing CodePaster
          post() functionality.

   \sa ExtensionSystem::PluginManager::getObjectByClassName, ExtensionSystem::invoke
   \sa VcsBase::VcsBaseEditorWidget
*/

CodePasterService::CodePasterService(QObject *parent) :
    QObject(parent)
{
}

void CodePasterService::postText(const QString &text, const QString &mimeType)
{
    QTC_ASSERT(CodepasterPlugin::instance(), return);
    CodepasterPlugin::instance()->post(text, mimeType);
}

void CodePasterService::postCurrentEditor()
{
    QTC_ASSERT(CodepasterPlugin::instance(), return);
    CodepasterPlugin::instance()->postEditor();
}

void CodePasterService::postClipboard()
{
    QTC_ASSERT(CodepasterPlugin::instance(), return);
    CodepasterPlugin::instance()->postClipboard();
}

// ---------- CodepasterPlugin
CodepasterPlugin *CodepasterPlugin::m_instance = 0;

CodepasterPlugin::CodepasterPlugin() :
    m_settings(new Settings),
    m_postEditorAction(0), m_postClipboardAction(0), m_fetchAction(0)
{
    CodepasterPlugin::m_instance = this;
}

CodepasterPlugin::~CodepasterPlugin()
{
    delete m_urlOpen;
    qDeleteAll(m_protocols);
    CodepasterPlugin::m_instance = 0;
}

bool CodepasterPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    // Create the globalcontext list to register actions accordingly
    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    // Create the settings Page
    m_settings->fromSettings(Core::ICore::settings());
    SettingsPage *settingsPage = new SettingsPage(m_settings);
    addAutoReleasedObject(settingsPage);

    // Create the protocols and append them to the Settings
    const QSharedPointer<NetworkAccessManagerProxy> networkAccessMgrProxy(new NetworkAccessManagerProxy);
    Protocol *protos[] =  { new PasteBinDotComProtocol(networkAccessMgrProxy),
                            new PasteBinDotCaProtocol(networkAccessMgrProxy),
                            new KdePasteProtocol(networkAccessMgrProxy),
                            new CodePasterProtocol(networkAccessMgrProxy),
                            new FileShareProtocol
                           };
    const int count = sizeof(protos) / sizeof(Protocol *);
    for (int i = 0; i < count; ++i) {
        connect(protos[i], SIGNAL(pasteDone(QString)), this, SLOT(finishPost(QString)));
        connect(protos[i], SIGNAL(fetchDone(QString,QString,bool)),
                this, SLOT(finishFetch(QString,QString,bool)));
        settingsPage->addProtocol(protos[i]->name());
        if (protos[i]->hasSettings())
            addAutoReleasedObject(protos[i]->settingsPage());
        m_protocols.append(protos[i]);
    }

    m_urlOpen = new UrlOpenProtocol(networkAccessMgrProxy);
    connect(m_urlOpen, SIGNAL(fetchDone(QString,QString,bool)),
            this, SLOT(finishFetch(QString,QString,bool)));

    //register actions

    Core::ActionContainer *toolsContainer =
        Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *cpContainer =
        Core::ActionManager::createMenu(Core::Id("CodePaster"));
    cpContainer->menu()->setTitle(tr("&Code Pasting"));
    toolsContainer->addMenu(cpContainer);

    Core::Command *command;

    m_postEditorAction = new QAction(tr("Paste Snippet..."), this);
    command = Core::ActionManager::registerAction(m_postEditorAction, "CodePaster.Post", globalcontext);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+P") : tr("Alt+C,Alt+P")));
    connect(m_postEditorAction, SIGNAL(triggered()), this, SLOT(postEditor()));
    cpContainer->addAction(command);

    m_postClipboardAction = new QAction(tr("Paste Clipboard..."), this);
    command = Core::ActionManager::registerAction(m_postClipboardAction, "CodePaster.PostClipboard", globalcontext);
    connect(m_postClipboardAction, SIGNAL(triggered()), this, SLOT(postClipboard()));
    cpContainer->addAction(command);

    m_fetchAction = new QAction(tr("Fetch Snippet..."), this);
    command = Core::ActionManager::registerAction(m_fetchAction, "CodePaster.Fetch", globalcontext);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+F") : tr("Alt+C,Alt+F")));
    connect(m_fetchAction, SIGNAL(triggered()), this, SLOT(fetch()));
    cpContainer->addAction(command);

    m_fetchUrlAction = new QAction(tr("Fetch from URL..."), this);
    command = Core::ActionManager::registerAction(m_fetchUrlAction, "CodePaster.FetchUrl", globalcontext);
    connect(m_fetchUrlAction, SIGNAL(triggered()), this, SLOT(fetchUrl()));
    cpContainer->addAction(command);

    addAutoReleasedObject(new CodePasterService);

    return true;
}

void CodepasterPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag CodepasterPlugin::aboutToShutdown()
{
    // Delete temporary, fetched files
    foreach (const QString &fetchedSnippet, m_fetchedSnippets) {
        QFile file(fetchedSnippet);
        if (file.exists())
            file.remove();
    }
    return SynchronousShutdown;
}

void CodepasterPlugin::postEditor()
{
    QString data;
    QString mimeType;
    if (IEditor *editor = EditorManager::currentEditor()) {
        if (ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor)) {
            data = textEditor->selectedText();
            if (data.isEmpty())
                data = textEditor->contents();
            mimeType = textEditor->document()->mimeType();
        }
    }
    post(data, mimeType);
}

void CodepasterPlugin::postClipboard()
{
    QString subtype = QLatin1String("plain");
    const QString text = qApp->clipboard()->text(subtype, QClipboard::Clipboard);
    if (!text.isEmpty())
        post(text, QString());
}

static inline void fixSpecialCharacters(QString &data)
{
    QChar *uc = data.data();
    QChar *e = uc + data.size();

    for (; uc != e; ++uc) {
        switch (uc->unicode()) {
        case 0xfdd0: // QTextBeginningOfFrame
        case 0xfdd1: // QTextEndOfFrame
        case QChar::ParagraphSeparator:
        case QChar::LineSeparator:
            *uc = QLatin1Char('\n');
            break;
        case QChar::Nbsp:
            *uc = QLatin1Char(' ');
            break;
        default:
            break;
        }
    }
}

void CodepasterPlugin::post(QString data, const QString &mimeType)
{
    fixSpecialCharacters(data);

    const QString username = m_settings->username;

    PasteView view(m_protocols, mimeType, ICore::mainWindow());
    view.setProtocol(m_settings->protocol);

    const FileDataList diffChunks = splitDiffToFiles(data);
    const int dialogResult = diffChunks.isEmpty() ?
        view.show(username, QString(), QString(), data) :
        view.show(username, QString(), QString(), diffChunks);
    // Save new protocol in case user changed it.
    if (dialogResult == QDialog::Accepted
        && m_settings->protocol != view.protocol()) {
        m_settings->protocol = view.protocol();
        m_settings->toSettings(Core::ICore::settings());
    }
}

void CodepasterPlugin::fetchUrl()
{
    QUrl url;
    do {
        bool ok = true;
        url = QUrl(QInputDialog::getText(0, tr("Fetch from URL"), tr("Enter URL:"), QLineEdit::Normal, QString(), &ok));
        if (!ok)
            return;
    } while (!url.isValid());
    m_urlOpen->fetch(url.toString());
}

void CodepasterPlugin::fetch()
{
    PasteSelectDialog dialog(m_protocols, ICore::mainWindow());
    dialog.setProtocol(m_settings->protocol);

    if (dialog.exec() != QDialog::Accepted)
        return;
    // Save new protocol in case user changed it.
    if (m_settings->protocol != dialog.protocol()) {
        m_settings->protocol = dialog.protocol();
        m_settings->toSettings(Core::ICore::settings());
    }

    const QString pasteID = dialog.pasteId();
    if (pasteID.isEmpty())
        return;
    Protocol *protocol = m_protocols[dialog.protocolIndex()];
    if (Protocol::ensureConfiguration(protocol))
        protocol->fetch(pasteID);
}

void CodepasterPlugin::finishPost(const QString &link)
{
    if (m_settings->copyToClipboard)
        QApplication::clipboard()->setText(link);
    ICore::messageManager()->printToOutputPane(link, m_settings->displayOutput);
}

// Extract the characters that can be used for a file name from a title
// "CodePaster.com-34" -> "CodePastercom34".
static inline QString filePrefixFromTitle(const QString &title)
{
    QString rc;
    const int titleSize = title.size();
    rc.reserve(titleSize);
    for (int i = 0; i < titleSize; i++)
        if (title.at(i).isLetterOrNumber())
            rc.append(title.at(i));
    if (rc.isEmpty()) {
        rc = QLatin1String("qtcreator");
    } else {
        if (rc.size() > 15)
            rc.truncate(15);
    }
    return rc;
}

// Return a temp file pattern with extension or not
static inline QString tempFilePattern(const QString &prefix, const QString &extension)
{
    // Get directory
    QString pattern = QDir::tempPath();
    if (!pattern.endsWith(QDir::separator()))
        pattern.append(QDir::separator());
    // Prefix, placeholder, extension
    pattern += prefix;
    pattern += QLatin1String("_XXXXXX.");
    pattern += extension;
    return pattern;
}

void CodepasterPlugin::finishFetch(const QString &titleDescription,
                                   const QString &content,
                                   bool error)
{
    Core::MessageManager *messageManager = ICore::messageManager();
    // Failure?
    if (error) {
        messageManager->printToOutputPane(content, true);
        return;
    }
    if (content.isEmpty()) {
        messageManager->printToOutputPane(tr("Empty snippet received for \"%1\".").arg(titleDescription), true);
        return;
    }
    // If the mime type has a preferred suffix (cpp/h/patch...), use that for
    // the temporary file. This is to make it more convenient to "Save as"
    // for the user and also to be able to tell a patch or diff in the VCS plugins
    // by looking at the file name of DocumentManager::currentFile() without expensive checking.
    // Default to "txt".
    QByteArray byteContent = content.toUtf8();
    QString suffix;
    if (const Core::MimeType mimeType = Core::ICore::mimeDatabase()->findByData(byteContent))
        suffix = mimeType.preferredSuffix();
    if (suffix.isEmpty())
         suffix = QLatin1String("txt");
    const QString filePrefix = filePrefixFromTitle(titleDescription);
    Utils::TempFileSaver saver(tempFilePattern(filePrefix, suffix));
    saver.setAutoRemove(false);
    saver.write(byteContent);
    if (!saver.finalize()) {
        messageManager->printToOutputPane(saver.errorString());
        return;
    }
    const QString fileName = saver.fileName();
    m_fetchedSnippets.push_back(fileName);
    // Open editor with title.
    Core::IEditor *editor = EditorManager::openEditor(fileName, Core::Id(), EditorManager::ModeSwitch);
    QTC_ASSERT(editor, return);
    editor->setDisplayName(titleDescription);
}

CodepasterPlugin *CodepasterPlugin::instance()
{
    return m_instance;
}

} // namespace CodePaster

Q_EXPORT_PLUGIN(CodePaster::CodepasterPlugin)
