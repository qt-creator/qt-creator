// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpasterplugin.h"

#include "cpastertr.h"
#include "dpastedotcomprotocol.h"
#include "fileshareprotocol.h"
#include "pastebindotcomprotocol.h"
#include "pasteselectdialog.h"
#include "pasteview.h"
#include "settings.h"
#include "urlopenprotocol.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

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

class CodePasterPluginPrivate : public QObject
{
public:
    CodePasterPluginPrivate();

    void post(CodePasterPlugin::PasteSources pasteSources);
    void post(QString data, const QString &mimeType);

    void pasteSnippet();
    void fetch();
    void finishPost(const QString &link);
    void finishFetch(const QString &titleDescription,
                     const QString &content,
                     bool error);

    void fetchUrl();

    QAction *m_postEditorAction = nullptr;
    QAction *m_fetchAction = nullptr;
    QAction *m_fetchUrlAction = nullptr;

    PasteBinDotComProtocol pasteBinProto;
    FileShareProtocol fileShareProto;
    DPasteDotComProtocol dpasteProto;

    const QList<Protocol *> m_protocols {
        &pasteBinProto,
        &fileShareProto,
        &dpasteProto
    };

    QStringList m_fetchedSnippets;

    UrlOpenProtocol m_urlOpen;

    CodePasterServiceImpl m_service{this};
};

/*!
   \class CodePaster::Service
   \brief The CodePaster::Service class is a service registered with PluginManager
   that provides CodePaster \c post() functionality.
*/

CodePasterServiceImpl::CodePasterServiceImpl(CodePasterPluginPrivate *d)
    : d(d)
{}

void CodePasterServiceImpl::postText(const QString &text, const QString &mimeType)
{
    d->post(text, mimeType);
}

void CodePasterServiceImpl::postCurrentEditor()
{
    d->post(CodePasterPlugin::PasteEditor);
}

void CodePasterServiceImpl::postClipboard()
{
    d->post(CodePasterPlugin::PasteClipboard);
}

// ---------- CodepasterPlugin

CodePasterPlugin::~CodePasterPlugin()
{
    delete d;
}

void CodePasterPlugin::initialize()
{
    d = new CodePasterPluginPrivate;
}

CodePasterPluginPrivate::CodePasterPluginPrivate()
{
    // Connect protocols
    if (!m_protocols.isEmpty()) {
        for (Protocol *proto : m_protocols) {
            settings().protocols.addOption(proto->name());
            connect(proto, &Protocol::pasteDone, this, &CodePasterPluginPrivate::finishPost);
            connect(proto, &Protocol::fetchDone, this, &CodePasterPluginPrivate::finishFetch);
        }
        settings().protocols.setDefaultValue(m_protocols.at(0)->name());
    }

    // Create the settings Page
    settings().readSettings();

    connect(&m_urlOpen, &Protocol::fetchDone, this, &CodePasterPluginPrivate::finishFetch);

    //register actions

    ActionContainer *toolsContainer = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *cpContainer = ActionManager::createMenu("CodePaster");
    cpContainer->menu()->setTitle(Tr::tr("&Code Pasting"));
    toolsContainer->addMenu(cpContainer);

    Command *command;

    m_postEditorAction = new QAction(Tr::tr("Paste Snippet..."), this);
    command = ActionManager::registerAction(m_postEditorAction, "CodePaster.Post");
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+C,Meta+P") : Tr::tr("Alt+C,Alt+P")));
    connect(m_postEditorAction, &QAction::triggered, this, &CodePasterPluginPrivate::pasteSnippet);
    cpContainer->addAction(command);

    m_fetchAction = new QAction(Tr::tr("Fetch Snippet..."), this);
    command = ActionManager::registerAction(m_fetchAction, "CodePaster.Fetch");
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+C,Meta+F") : Tr::tr("Alt+C,Alt+F")));
    connect(m_fetchAction, &QAction::triggered, this, &CodePasterPluginPrivate::fetch);
    cpContainer->addAction(command);

    m_fetchUrlAction = new QAction(Tr::tr("Fetch from URL..."), this);
    command = ActionManager::registerAction(m_fetchUrlAction, "CodePaster.FetchUrl");
    connect(m_fetchUrlAction, &QAction::triggered, this, &CodePasterPluginPrivate::fetchUrl);
    cpContainer->addAction(command);
}

ExtensionSystem::IPlugin::ShutdownFlag CodePasterPlugin::aboutToShutdown()
{
    // Delete temporary, fetched files
    for (const QString &fetchedSnippet : std::as_const(d->m_fetchedSnippets)) {
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
    if (auto textEditor = qobject_cast<const BaseTextEditor *>(editor))
        data = textEditor->selectedText();
    if (data.isEmpty()) {
        if (auto textDocument = qobject_cast<const TextDocument *>(document)) {
            data = textDocument->plainText();
        } else {
            const QVariant textV = document->property("plainText"); // Diff Editor.
            if (textV.typeId() == QMetaType::QString)
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

void CodePasterPluginPrivate::post(CodePasterPlugin::PasteSources pasteSources)
{
    QString data;
    QString mimeType;
    if (pasteSources & CodePasterPlugin::PasteEditor)
        textFromCurrentEditor(&data, &mimeType);
    if (data.isEmpty() && (pasteSources & CodePasterPlugin::PasteClipboard)) {
        QString subType = "plain";
        data = QGuiApplication::clipboard()->text(subType, QClipboard::Clipboard);
    }
    post(data, mimeType);
}

void CodePasterPluginPrivate::post(QString data, const QString &mimeType)
{
    fixSpecialCharacters(data);

    const QString username = settings().username();

    PasteView view(m_protocols, mimeType, ICore::dialogParent());
    view.setProtocol(settings().protocols.stringValue());

    const FileDataList diffChunks = splitDiffToFiles(data);
    const int dialogResult = diffChunks.isEmpty() ?
        view.show(username, {}, {}, settings().expiryDays(), data) :
        view.show(username, {}, {}, settings().expiryDays(), diffChunks);

    // Save new protocol in case user changed it.
    if (dialogResult == QDialog::Accepted && settings().protocols() != view.protocol()) {
        settings().protocols.setValue(view.protocol());
        settings().writeSettings();
    }
}

void CodePasterPluginPrivate::fetchUrl()
{
    QUrl url;
    do {
        bool ok = true;
        url = QUrl(QInputDialog::getText(ICore::dialogParent(), Tr::tr("Fetch from URL"), Tr::tr("Enter URL:"), QLineEdit::Normal, QString(), &ok));
        if (!ok)
            return;
    } while (!url.isValid());
    m_urlOpen.fetch(url.toString());
}

void CodePasterPluginPrivate::pasteSnippet()
{
    post(CodePasterPlugin::PasteEditor | CodePasterPlugin::PasteClipboard);
}

void CodePasterPluginPrivate::fetch()
{
    PasteSelectDialog dialog(m_protocols, ICore::dialogParent());
    dialog.setProtocol(settings().protocols.stringValue());

    if (dialog.exec() != QDialog::Accepted)
        return;
    // Save new protocol in case user changed it.
    if (settings().protocols() != dialog.protocol()) {
        settings().protocols.setValue(dialog.protocol());
        settings().writeSettings();
    }

    const QString pasteID = dialog.pasteId();
    if (pasteID.isEmpty())
        return;
    Protocol *protocol = m_protocols[dialog.protocol()];
    if (Protocol::ensureConfiguration(protocol))
        protocol->fetch(pasteID);
}

void CodePasterPluginPrivate::finishPost(const QString &link)
{
    if (settings().copyToClipboard())
        Utils::setClipboardAndSelection(link);

    if (settings().displayOutput())
        MessageManager::writeDisrupting(link);
    else
        MessageManager::writeFlashing(link);
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

void CodePasterPluginPrivate::finishFetch(const QString &titleDescription,
                                          const QString &content,
                                          bool error)
{
    // Failure?
    if (error) {
        MessageManager::writeDisrupting(content);
        return;
    }
    if (content.isEmpty()) {
        MessageManager::writeDisrupting(
            Tr::tr("Empty snippet received for \"%1\".").arg(titleDescription));
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
        MessageManager::writeDisrupting(saver.errorString());
        return;
    }
    const Utils::FilePath filePath = saver.filePath();
    m_fetchedSnippets.push_back(filePath.toString());
    // Open editor with title.
    IEditor *editor = EditorManager::openEditor(filePath);
    QTC_ASSERT(editor, return);
    editor->document()->setPreferredDisplayName(titleDescription);
}

} // namespace CodePaster
