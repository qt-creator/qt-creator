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

#include "cpasterplugin.h"

#include "pasteview.h"
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
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <QtPlugin>
#include <QDebug>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QMenu>
#include <QUrl>

using namespace Core;
using namespace TextEditor;

namespace CodePaster {

/*!
   \class CodePaster::Service
   \brief The CodePaster::Service class is a service registered with PluginManager
   that provides CodePaster \c post() functionality.
*/

CodePasterServiceImpl::CodePasterServiceImpl(QObject *parent) :
    QObject(parent)
{
}

void CodePasterServiceImpl::postText(const QString &text, const QString &mimeType)
{
    QTC_ASSERT(CodepasterPlugin::instance(), return);
    CodepasterPlugin::instance()->post(text, mimeType);
}

void CodePasterServiceImpl::postCurrentEditor()
{
    QTC_ASSERT(CodepasterPlugin::instance(), return);
    CodepasterPlugin::instance()->post(CodepasterPlugin::PasteEditor);
}

void CodePasterServiceImpl::postClipboard()
{
    QTC_ASSERT(CodepasterPlugin::instance(), return);
    CodepasterPlugin::instance()->post(CodepasterPlugin::PasteClipboard);
}

// ---------- CodepasterPlugin
CodepasterPlugin *CodepasterPlugin::m_instance = nullptr;

CodepasterPlugin::CodepasterPlugin() :
    m_settings(new Settings)
{
    CodepasterPlugin::m_instance = this;
}

CodepasterPlugin::~CodepasterPlugin()
{
    delete m_urlOpen;
    qDeleteAll(m_protocols);
    CodepasterPlugin::m_instance = nullptr;
}

bool CodepasterPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    // Create the settings Page
    m_settings->fromSettings(ICore::settings());
    SettingsPage *settingsPage = new SettingsPage(m_settings);
    addAutoReleasedObject(settingsPage);

    // Create the protocols and append them to the Settings
    Protocol *protos[] =  {new PasteBinDotComProtocol,
                           new PasteBinDotCaProtocol,
                           new KdePasteProtocol,
                           new FileShareProtocol
                          };
    const int count = sizeof(protos) / sizeof(Protocol *);
    for (int i = 0; i < count; ++i) {
        connect(protos[i], &Protocol::pasteDone, this, &CodepasterPlugin::finishPost);
        connect(protos[i], &Protocol::fetchDone, this, &CodepasterPlugin::finishFetch);
        settingsPage->addProtocol(protos[i]->name());
        if (protos[i]->hasSettings())
            addAutoReleasedObject(protos[i]->settingsPage());
        m_protocols.append(protos[i]);
    }

    m_urlOpen = new UrlOpenProtocol;
    connect(m_urlOpen, &Protocol::fetchDone, this, &CodepasterPlugin::finishFetch);

    //register actions

    ActionContainer *toolsContainer =
        ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *cpContainer =
        ActionManager::createMenu("CodePaster");
    cpContainer->menu()->setTitle(tr("&Code Pasting"));
    toolsContainer->addMenu(cpContainer);

    Command *command;

    m_postEditorAction = new QAction(tr("Paste Snippet..."), this);
    command = ActionManager::registerAction(m_postEditorAction, "CodePaster.Post");
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+P") : tr("Alt+C,Alt+P")));
    connect(m_postEditorAction, &QAction::triggered, this, &CodepasterPlugin::pasteSnippet);
    cpContainer->addAction(command);

    m_fetchAction = new QAction(tr("Fetch Snippet..."), this);
    command = ActionManager::registerAction(m_fetchAction, "CodePaster.Fetch");
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+F") : tr("Alt+C,Alt+F")));
    connect(m_fetchAction, &QAction::triggered, this, &CodepasterPlugin::fetch);
    cpContainer->addAction(command);

    m_fetchUrlAction = new QAction(tr("Fetch from URL..."), this);
    command = ActionManager::registerAction(m_fetchUrlAction, "CodePaster.FetchUrl");
    connect(m_fetchUrlAction, &QAction::triggered, this, &CodepasterPlugin::fetchUrl);
    cpContainer->addAction(command);

    addAutoReleasedObject(new CodePasterServiceImpl);

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

static inline void textFromCurrentEditor(QString *text, QString *mimeType)
{
    IEditor *editor = EditorManager::currentEditor();
    if (!editor)
        return;
    const IDocument *document = editor->document();
    QString data;
    if (const BaseTextEditor *textEditor = qobject_cast<const BaseTextEditor *>(editor))
        data = textEditor->selectedText();
    if (data.isEmpty()) {
        if (auto textDocument = qobject_cast<const TextDocument *>(document)) {
            data = textDocument->plainText();
        } else {
            const QVariant textV = document->property("plainText"); // Diff Editor.
            if (textV.type() == QVariant::String)
                data = textV.toString();
        }
    }
    if (!data.isEmpty()) {
        *text = data;
        *mimeType = document->mimeType();
    }
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

void CodepasterPlugin::post(PasteSources pasteSources)
{
    QString data;
    QString mimeType;
    if (pasteSources & PasteEditor)
        textFromCurrentEditor(&data, &mimeType);
    if (data.isEmpty() && (pasteSources & PasteClipboard)) {
        QString subType = QStringLiteral("plain");
        data = qApp->clipboard()->text(subType, QClipboard::Clipboard);
    }
    post(data, mimeType);
}

void CodepasterPlugin::post(QString data, const QString &mimeType)
{
    fixSpecialCharacters(data);

    const QString username = m_settings->username;

    PasteView view(m_protocols, mimeType, ICore::dialogParent());
    view.setProtocol(m_settings->protocol);

    const FileDataList diffChunks = splitDiffToFiles(data);
    const int dialogResult = diffChunks.isEmpty() ?
        view.show(username, QString(), QString(), m_settings->expiryDays, data) :
        view.show(username, QString(), QString(), m_settings->expiryDays, diffChunks);
    // Save new protocol in case user changed it.
    if (dialogResult == QDialog::Accepted
        && m_settings->protocol != view.protocol()) {
        m_settings->protocol = view.protocol();
        m_settings->toSettings(ICore::settings());
    }
}

void CodepasterPlugin::fetchUrl()
{
    QUrl url;
    do {
        bool ok = true;
        url = QUrl(QInputDialog::getText(ICore::dialogParent(), tr("Fetch from URL"), tr("Enter URL:"), QLineEdit::Normal, QString(), &ok));
        if (!ok)
            return;
    } while (!url.isValid());
    m_urlOpen->fetch(url.toString());
}

void CodepasterPlugin::pasteSnippet()
{
    post(PasteEditor | PasteClipboard);
}

void CodepasterPlugin::fetch()
{
    PasteSelectDialog dialog(m_protocols, ICore::dialogParent());
    dialog.setProtocol(m_settings->protocol);

    if (dialog.exec() != QDialog::Accepted)
        return;
    // Save new protocol in case user changed it.
    if (m_settings->protocol != dialog.protocol()) {
        m_settings->protocol = dialog.protocol();
        m_settings->toSettings(ICore::settings());
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
    MessageManager::write(link, m_settings->displayOutput ? MessageManager::ModeSwitch : MessageManager::Silent);
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
    QString pattern = Utils::TemporaryDirectory::masterDirectoryPath();
    const QChar slash = QLatin1Char('/');
    if (!pattern.endsWith(slash))
        pattern.append(slash);
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
    // Failure?
    if (error) {
        MessageManager::write(content);
        return;
    }
    if (content.isEmpty()) {
        MessageManager::write(tr("Empty snippet received for \"%1\".").arg(titleDescription));
        return;
    }
    // If the mime type has a preferred suffix (cpp/h/patch...), use that for
    // the temporary file. This is to make it more convenient to "Save as"
    // for the user and also to be able to tell a patch or diff in the VCS plugins
    // by looking at the file name of DocumentManager::currentFile() without expensive checking.
    // Default to "txt".
    QByteArray byteContent = content.toUtf8();
    QString suffix;
    const Utils::MimeType mimeType = Utils::mimeTypeForData(byteContent);
    if (mimeType.isValid())
        suffix = mimeType.preferredSuffix();
    if (suffix.isEmpty())
         suffix = QLatin1String("txt");
    const QString filePrefix = filePrefixFromTitle(titleDescription);
    Utils::TempFileSaver saver(tempFilePattern(filePrefix, suffix));
    saver.setAutoRemove(false);
    saver.write(byteContent);
    if (!saver.finalize()) {
        MessageManager::write(saver.errorString());
        return;
    }
    const QString fileName = saver.fileName();
    m_fetchedSnippets.push_back(fileName);
    // Open editor with title.
    IEditor *editor = EditorManager::openEditor(fileName);
    QTC_ASSERT(editor, return);
    editor->document()->setPreferredDisplayName(titleDescription);
}

CodepasterPlugin *CodepasterPlugin::instance()
{
    return m_instance;
}

} // namespace CodePaster
