// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcpkgmanifesteditor.h"

#include "vcpkgconstants.h"
#include "vcpkgsearch.h"
#include "vcpkgsettings.h"
#include "vcpkgtr.h"

#include <coreplugin/icore.h>

#include <utils/utilsicons.h>

#include <texteditor/textdocument.h>

#include <QToolBar>

namespace Vcpkg::Internal {

class VcpkgManifestEditorWidget : public TextEditor::TextEditorWidget
{
public:
    VcpkgManifestEditorWidget()
    {
        m_searchPkgAction = toolBar()->addAction(Utils::Icons::ZOOM_TOOLBAR.icon(),
                                                        Tr::tr("Search package..."));
        connect(m_searchPkgAction, &QAction::triggered, this, [this] {
            const Search::VcpkgManifest package = Search::showVcpkgPackageSearchDialog();
            if (!package.name.isEmpty())
                textCursor().insertText(package.name);
        });
        updateToolBar();

        QAction *optionsAction = toolBar()->addAction(Utils::Icons::SETTINGS_TOOLBAR.icon(),
                                                      Core::ICore::msgShowOptionsDialog());
        connect(optionsAction, &QAction::triggered, [] {
            Core::ICore::showOptionsDialog(Constants::TOOLSSETTINGSPAGE_ID);
        });

        connect(&settings().vcpkgRoot, &Utils::BaseAspect::changed,
                this, &VcpkgManifestEditorWidget::updateToolBar);
    }

    void updateToolBar()
    {
        Utils::FilePath vcpkg = settings().vcpkgRoot().pathAppended("vcpkg").withExecutableSuffix();
        m_searchPkgAction->setEnabled(vcpkg.isExecutableFile());
    }

private:
    QAction *m_searchPkgAction;
};

static TextEditor::TextDocument *createVcpkgManifestDocument()
{
    auto doc = new TextEditor::TextDocument;
    doc->setId(Constants::VCPKGMANIFEST_EDITOR_ID);
    return doc;
}

VcpkgManifestEditorFactory::VcpkgManifestEditorFactory()
{
    setId(Constants::VCPKGMANIFEST_EDITOR_ID);
    setDisplayName(Tr::tr("Vcpkg Manifest Editor"));
    addMimeType(Constants::VCPKGMANIFEST_MIMETYPE);
    setDocumentCreator(createVcpkgManifestDocument);
    setEditorWidgetCreator([] { return new VcpkgManifestEditorWidget; });
    setUseGenericHighlighter(true);
}

} // namespace Vcpkg::Internal
