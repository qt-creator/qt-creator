// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codepasterservice.h"
#include "cpasterconstants.h"
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
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/globaltasktree.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QDebug>
#include <QClipboard>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMenu>
#include <QUrl>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace CodePaster {

class CodePasterPluginPrivate;

enum PasteSource {
    PasteEditor = 0x1,
    PasteClipboard = 0x2
};

Q_DECLARE_FLAGS(PasteSources, PasteSource)
Q_DECLARE_OPERATORS_FOR_FLAGS(PasteSources)

class CodePasterServiceImpl final : public QObject, public CodePaster::Service
{
    Q_OBJECT
    Q_INTERFACES(CodePaster::Service)

public:
    explicit CodePasterServiceImpl(CodePasterPluginPrivate *d);

    void postText(const QString &text, const QString &mimeType) final;
    void postCurrentEditor() final;
    void postClipboard() final;

private:
    CodePasterPluginPrivate *d = nullptr;
};

class CodePasterPluginPrivate : public QObject
{
public:
    CodePasterPluginPrivate();
    ~CodePasterPluginPrivate();

    void post(PasteSources pasteSources);
    void pasteSnippet();

    void fetch();
    void fetchUrl();
    void fetchId(const QString &pasteId, Protocol *protocol);

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

    QSingleTaskTreeRunner m_taskTreeRunner;
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
    const auto pasteInputData = executePasteDialog(d->m_protocols, text, mimeType);
    if (!pasteInputData)
        return;

    const auto pasteHandler = [](const QString &link) {
        if (settings().copyToClipboard())
            Utils::setClipboardAndSelection(link);

        if (settings().displayOutput())
            MessageManager::writeDisrupting(link);
        else
            MessageManager::writeFlashing(link);
    };

    Protocol *protocol = d->m_protocols[settings().protocols()];
    GlobalTaskTree::start({protocol->pasteRecipe(*pasteInputData, pasteHandler)});
}

void CodePasterServiceImpl::postCurrentEditor()
{
    d->post(PasteEditor);
}

void CodePasterServiceImpl::postClipboard()
{
    d->post(PasteClipboard);
}

// CodepasterPlugin

CodePasterPluginPrivate::CodePasterPluginPrivate()
{
    // Connect protocols
    if (!m_protocols.isEmpty()) {
        for (Protocol *proto : m_protocols)
            settings().protocols.addOption(proto->name());
        settings().protocols.setDefaultValue(m_protocols.at(0)->name());
    }

    // Create the settings Page
    settings().readSettings();

    //register actions

    ActionContainer *toolsContainer = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    const Id menu = "CodePaster";
    ActionContainer *cpContainer = ActionManager::createMenu(menu);
    cpContainer->menu()->setTitle(Tr::tr("&Code Pasting"));
    toolsContainer->addMenu(cpContainer);

    ActionBuilder(this, "CodePaster.Post")
        .setText(Tr::tr("Paste Snippet..."))
        .setDefaultKeySequence(Tr::tr("Meta+C,Meta+P"), Tr::tr("Alt+C,Alt+P"))
        .addToContainer(menu)
        .addOnTriggered(this, &CodePasterPluginPrivate::pasteSnippet);

    ActionBuilder(this, "CodePaster.Fetch")
        .setText(Tr::tr("Fetch Snippet..."))
        .setDefaultKeySequence(Tr::tr("Meta+C,Meta+F"), Tr::tr("Alt+C,Alt+F"))
        .addToContainer(menu)
        .addOnTriggered(this, &CodePasterPluginPrivate::fetch);

    ActionBuilder(this, "CodePaster.FetchUrl")
        .setText(Tr::tr("Fetch from URL..."))
        .addToContainer(menu)
        .addOnTriggered(this, &CodePasterPluginPrivate::fetchUrl);

    ExtensionSystem::PluginManager::addObject(&m_service);
}

CodePasterPluginPrivate::~CodePasterPluginPrivate()
{
    ExtensionSystem::PluginManager::removeObject(&m_service);
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

void CodePasterPluginPrivate::post(PasteSources pasteSources)
{
    QString data;
    QString mimeType;
    if (pasteSources & PasteEditor)
        textFromCurrentEditor(&data, &mimeType);
    if (data.isEmpty() && (pasteSources & PasteClipboard)) {
        QString subType = "plain";
        data = QGuiApplication::clipboard()->text(subType, QClipboard::Clipboard);
    }
    m_service.postText(data, mimeType);
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
    fetchId(url.toString(), &m_urlOpen);
}

void CodePasterPluginPrivate::pasteSnippet()
{
    post(PasteEditor | PasteClipboard);
}

void CodePasterPluginPrivate::fetch()
{
    const QString pasteId = executeFetchDialog(m_protocols);
    if (pasteId.isEmpty())
        return;
    Protocol *protocol = m_protocols[settings().protocols()];
    if (Protocol::ensureConfiguration(protocol))
        fetchId(pasteId, protocol);
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

void CodePasterPluginPrivate::fetchId(const QString &pasteId, Protocol *protocol)
{
    const auto fetchHandler = [this](const QString &titleDescription, const QString &content) {
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
        const QByteArray byteContent = content.toUtf8();
        QString suffix;
        const MimeType mimeType = mimeTypeForData(byteContent);
        if (mimeType.isValid())
            suffix = mimeType.preferredSuffix();
        if (suffix.isEmpty())
            suffix = QLatin1String("txt");

        const QString filePrefix = filePrefixFromTitle(titleDescription);
        TempFileSaver saver(tempFilePattern(filePrefix, suffix));
        saver.setAutoRemove(false);
        saver.write(byteContent);
        if (const Result<> res = saver.finalize(); !res) {
            MessageManager::writeDisrupting(res.error());
            return;
        }

        const FilePath filePath = saver.filePath();
        m_fetchedSnippets.push_back(filePath.toUrlishString());
        // Open editor with title.
        IEditor *editor = EditorManager::openEditor(filePath);
        QTC_ASSERT(editor, return);
        editor->document()->setPreferredDisplayName(titleDescription);
    };

    m_taskTreeRunner.start({protocol->fetchRecipe(pasteId, fetchHandler)});
}

// CodePasterPlugin

class CodePasterPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CodePaster.json")

public:
    ~CodePasterPlugin() final
    {
        delete d;
    }

private:
    void initialize() final
    {
        IOptionsPage::registerCategory(
            CodePaster::Constants::CPASTER_SETTINGS_CATEGORY,
            Tr::tr("Code Pasting"),
            ":/cpaster/images/settingscategory_cpaster.png");

        d = new CodePasterPluginPrivate;
    }

    ShutdownFlag aboutToShutdown() final
    {
        // Delete temporary, fetched files
        for (const QString &fetchedSnippet : std::as_const(d->m_fetchedSnippets)) {
            QFile file(fetchedSnippet);
            if (file.exists())
                file.remove();
        }
        return SynchronousShutdown;
    }

    CodePasterPluginPrivate *d = nullptr;
};

} // namespace CodePaster

#include "cpasterplugin.moc"
