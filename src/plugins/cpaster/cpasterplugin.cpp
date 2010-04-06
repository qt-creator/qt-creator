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

#include "cpasterplugin.h"

#include "splitter.h"
#include "pasteview.h"
#include "codepasterprotocol.h"
#include "pastebindotcomprotocol.h"
#include "pastebindotcaprotocol.h"
#include "pasteselectdialog.h"
#include "settingspage.h"
#include "settings.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <utils/qtcassert.h>
#include <texteditor/basetexteditor.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMenu>
#include <QtGui/QMainWindow>

using namespace CodePaster;
using namespace Core;
using namespace TextEditor;

CodepasterPlugin::CodepasterPlugin() :
    m_settings(new Settings),
    m_postEditorAction(0), m_postClipboardAction(0), m_fetchAction(0)
{
}

CodepasterPlugin::~CodepasterPlugin()
{
    qDeleteAll(m_protocols);
}

bool CodepasterPlugin::initialize(const QStringList &arguments, QString *error_message)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error_message)

    // Create the globalcontext list to register actions accordingly
    QList<int> globalcontext;
    globalcontext << UniqueIDManager::instance()->uniqueIdentifier(Core::Constants::C_GLOBAL);

    // Create the settings Page
    m_settings->fromSettings(Core::ICore::instance()->settings());
    SettingsPage *settingsPage = new SettingsPage(m_settings);
    addAutoReleasedObject(settingsPage);

    // Create the protocols and append them to the Settings
    const QSharedPointer<NetworkAccessManagerProxy> networkAccessMgrProxy(new NetworkAccessManagerProxy);
    Protocol *protos[] =  { new PasteBinDotComProtocol(networkAccessMgrProxy),
                            new PasteBinDotCaProtocol(networkAccessMgrProxy),
                            new CodePasterProtocol(networkAccessMgrProxy)
                           };
    const int count = sizeof(protos) / sizeof(Protocol *);
    for(int i = 0; i < count; ++i) {
        connect(protos[i], SIGNAL(pasteDone(QString)), this, SLOT(finishPost(QString)));
        connect(protos[i], SIGNAL(fetchDone(QString,QString,bool)),
                this, SLOT(finishFetch(QString,QString,bool)));
        settingsPage->addProtocol(protos[i]->name());
        if (protos[i]->hasSettings())
            addAutoReleasedObject(protos[i]->settingsPage());
        m_protocols.append(protos[i]);
    }

    //register actions
    Core::ActionManager *actionManager = ICore::instance()->actionManager();

    Core::ActionContainer *toolsContainer =
        actionManager->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *cpContainer =
        actionManager->createMenu(QLatin1String("CodePaster"));
    cpContainer->menu()->setTitle(tr("&Code Pasting"));
    toolsContainer->addMenu(cpContainer);

    Core::Command *command;

    m_postEditorAction = new QAction(tr("Paste Snippet..."), this);
    command = actionManager->registerAction(m_postEditorAction, "CodePaster.Post", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+P")));
    connect(m_postEditorAction, SIGNAL(triggered()), this, SLOT(postEditor()));
    cpContainer->addAction(command);

    m_postClipboardAction = new QAction(tr("Paste Clipboard..."), this);
    command = actionManager->registerAction(m_postClipboardAction, "CodePaster.PostClipboard", globalcontext);
    connect(m_postClipboardAction, SIGNAL(triggered()), this, SLOT(postClipboard()));
    cpContainer->addAction(command);

    m_fetchAction = new QAction(tr("Fetch Snippet..."), this);
    command = actionManager->registerAction(m_fetchAction, "CodePaster.Fetch", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+F")));
    connect(m_fetchAction, SIGNAL(triggered()), this, SLOT(fetch()));
    cpContainer->addAction(command);

    return true;
}

void CodepasterPlugin::extensionsInitialized()
{
}

void CodepasterPlugin::shutdown()
{
    // Delete temporary, fetched files
    foreach(const QString &fetchedSnippet, m_fetchedSnippets) {
        QFile file(fetchedSnippet);
        if (file.exists())
            file.remove();
    }
}

void CodepasterPlugin::postEditor()
{
    const IEditor* editor = EditorManager::instance()->currentEditor();
    const BaseTextEditorEditable *textEditor = qobject_cast<const BaseTextEditorEditable *>(editor);
    if (!textEditor)
        return;

    QString data = textEditor->selectedText();
    if (data.isEmpty())
        data = textEditor->contents();
    if (!data.isEmpty())
        post(data, textEditor->editor()->mimeType());
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
    FileDataList lst = splitDiffToFiles(data.toLatin1());
    QString username = m_settings->username;
    QString description;
    QString comment;
    QString protocolName;

    PasteView view(m_protocols, 0);
    view.setProtocol(m_settings->protocol);

    if (!view.show(username, description, comment, lst))
        return; // User canceled post

    username = view.user();
    description = view.description();
    comment = view.comment();
    data = view.content();
    protocolName = view.protocol();
    foreach(Protocol *protocol, m_protocols) {
        if (protocol->name() == protocolName) {
            const Protocol::ContentType ct = Protocol::contentType(mimeType);
            protocol->paste(data, ct, username, comment, description);
            break;
        }
    }
}

void CodepasterPlugin::fetch()
{
    PasteSelectDialog dialog(m_protocols, ICore::instance()->mainWindow());
    dialog.setProtocol(m_settings->protocol);

    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString pasteID = dialog.pasteId();
    if (pasteID.isEmpty())
        return;
    m_protocols[dialog.protocolIndex()]->fetch(pasteID);
}

void CodepasterPlugin::finishPost(const QString &link)
{
    if (m_settings->copyToClipboard)
        QApplication::clipboard()->setText(link);
    ICore::instance()->messageManager()->printToOutputPane(link,
                                                           m_settings->displayOutput);
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
static inline QString tempFilePattern(const QString &prefix,
                                      const QString &extension = QString())
{
    // Get directory
    QString pattern = QDir::tempPath();
    if (!pattern.endsWith(QDir::separator()))
        pattern.append(QDir::separator());
    // Prefix, placeholder, extension
    pattern += prefix;
    pattern += QLatin1String("_XXXXXX");
    if (!extension.isEmpty()) {
       pattern += QLatin1Char('.');
       pattern += extension;
    }
    return pattern;
}

typedef QSharedPointer<QTemporaryFile> TemporaryFilePtr;

// Write an a temporary file.
TemporaryFilePtr writeTemporaryFile(const QString &namePattern,
                                    const QString &contents,
                                    QString *errorMessage)
{
    TemporaryFilePtr tempFile(new QTemporaryFile(namePattern));
    if (!tempFile->open()) {
        *errorMessage = QString::fromLatin1("Unable to open temporary file %1").arg(tempFile->errorString());
        return TemporaryFilePtr();
    }
    tempFile->write(contents.toUtf8());
    tempFile->close();
    return tempFile;
}

void CodepasterPlugin::finishFetch(const QString &titleDescription,
                                   const QString &content,
                                   bool error)
{
    Core::MessageManager *messageManager = ICore::instance()->messageManager();
    // Failure?
    if (error) {
        messageManager->printToOutputPane(content, true);
        return;
    }
    if (content.isEmpty()) {
        messageManager->printToOutputPane(tr("Empty snippet received for \"%1\".").arg(titleDescription), true);
        return;
    }
    // Write the file out and do a mime type detection on the content. Note
    // that for the initial detection, there must not be a suffix
    // as we want mime type detection to trigger on the content and not on
    // higher-prioritized suffixes.
    const QString filePrefix = filePrefixFromTitle(titleDescription);
    QString errorMessage;
    TemporaryFilePtr tempFile = writeTemporaryFile(tempFilePattern(filePrefix), content, &errorMessage);
    if (tempFile.isNull()) {
        messageManager->printToOutputPane(errorMessage);
        return;
    }
    // If the mime type has a preferred suffix (cpp/h/patch...), use that for
    // the temporary file. This is to make it more convenient to "Save as"
    // for the user and also to be able to tell a patch or diff in the VCS plugins
    // by looking at the file name of FileManager::currentFile() without expensive checking.
    // Default to "txt".
    QString suffix;
    if (const Core::MimeType mimeType = Core::ICore::instance()->mimeDatabase()->findByFile(QFileInfo(tempFile->fileName())))
        suffix = mimeType.preferredSuffix();
    if (suffix.isEmpty())
         suffix = QLatin1String("txt");
    // Write out with extension.
    tempFile = writeTemporaryFile(tempFilePattern(filePrefix, suffix), content, &errorMessage);
    if (tempFile.isNull()) {
        messageManager->printToOutputPane(errorMessage);
        return;
    }
    // Keep the file and store in list of files to be removed.
    tempFile->setAutoRemove(false);
    const QString fileName = tempFile->fileName();
    m_fetchedSnippets.push_back(fileName);
    // Open editor with title.
    Core::IEditor* editor = EditorManager::instance()->openEditor(fileName);
    QTC_ASSERT(editor, return)
    editor->setDisplayName(titleDescription);
    EditorManager::instance()->activateEditor(editor);
}

Q_EXPORT_PLUGIN(CodepasterPlugin)
