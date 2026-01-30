// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "swiftproject.h"
#include "swifttr.h"
#include "utils/infobar.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/processprogress.h>
#include <extensionsystem/iplugin.h>
#include <languageclient/languageclientmanager.h>
#include <utils/environment.h>
#include <utils/globaltasktree.h>
#include <utils/qtcprocess.h>

using namespace Core;
using namespace LanguageClient;
using namespace QtTaskTree;
using namespace Utils;

namespace Swift::Internal {

class SwiftPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Swift.json")

public:
    SwiftPlugin() = default;
    ~SwiftPlugin() final = default;

    void initialize() final;

    void editorOpened(Core::IEditor *editor);
};

void SwiftPlugin::initialize()
{
    setupSwiftProject();
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &SwiftPlugin::editorOpened);

}

static QStringList swiftMimeTypes()
{
    static QStringList mimeTypes = {
        "text/x-swift",
        "text/x-swift-manifest"
    };
    return mimeTypes;
}

constexpr char setupSwiftLsInfoBarId[] = "LanguageClient::setupSwiftLs";

static void setupSwiftLanguageServer(IDocument *document)
{
    InfoBar *infoBar = document->infoBar();
    if (!infoBar->canInfoBeAdded(setupSwiftLsInfoBarId))
        return;

    // check if it is already configured
    const QList<BaseSettings *> settings = LanguageClientManager::currentSettings();
    for (BaseSettings *setting : settings) {
        if (setting->isValid() && setting->m_languageFilter.isSupported(document))
            return;
    }

    const FilePath sourcekitLsp = Environment::systemEnvironment().searchInPath("sourcekit-lsp");
    if (!sourcekitLsp.isExecutableFile())
        return;

    const QString message
        = Tr::tr("Set up Swift language server (%1).").arg(sourcekitLsp.toUserOutput());
    InfoBarEntry info(setupSwiftLsInfoBarId, message, InfoBarEntry::GlobalSuppression::Enabled);
    info.addCustomButton(Tr::tr("Set Up"), [=]() {
        const QList<IDocument *> &openedDocuments = DocumentModel::openedDocuments();
        for (IDocument *doc : openedDocuments)
            doc->infoBar()->removeInfo(setupSwiftLsInfoBarId);

        auto settings = new StdIOSettings();

        settings->executable.setValue(sourcekitLsp);
        settings->name.setValue(Tr::tr("Swift Language Server"));
        settings->m_languageFilter.mimeTypes = swiftMimeTypes();
        settings->m_startBehavior = BaseSettings::RequiresProject;
        LanguageClientSettings::addSettings(settings);
        LanguageClientManager::applySettings();
    });
    infoBar->addInfo(info);
}


void SwiftPlugin::editorOpened(IEditor *editor)
{
    if (!editor)
        return;

    IDocument *doc = editor->document();
    if (!doc)
        return;

    const QString mimeType = doc->mimeType();
    if (!swiftMimeTypes().contains(mimeType))
        return;

    setupSwiftLanguageServer(doc);
}

} // namespace Swift::Internal

#include "swiftplugin.moc"
