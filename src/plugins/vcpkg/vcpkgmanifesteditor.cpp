// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcpkgmanifesteditor.h"

#include "vcpkgconstants.h"
#include "vcpkgsearch.h"
#include "vcpkgsettings.h"
#include "vcpkgtr.h"

#include <coreplugin/icore.h>

#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <projectexplorer/projectexplorericons.h>

#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QToolBar>

namespace Vcpkg::Internal {

class CMakeCodeDialog : public QDialog
{
public:
    explicit CMakeCodeDialog(const QStringList &packages, QWidget *parent = nullptr);

private:
    static QString cmakeCodeForPackage(const QString &package);
    static QString cmakeCodeForPackages(const QStringList &packages);
};

CMakeCodeDialog::CMakeCodeDialog(const QStringList &packages, QWidget *parent)
    : QDialog(parent)
{
    resize(600, 600);

    auto codeBrowser = new QPlainTextEdit;
    const TextEditor::FontSettings &fs = TextEditor::TextEditorSettings::fontSettings();
    codeBrowser->setFont(fs.font());
    codeBrowser->setPlainText(cmakeCodeForPackages(packages));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);

    using namespace Layouting;
    Column {
        Tr::tr("Copy paste the required lines into your CMakeLists.txt:"),
        codeBrowser,
        buttonBox,
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString CMakeCodeDialog::cmakeCodeForPackage(const QString &package)
{
    QString result;

    const Utils::FilePath usageFile = settings().vcpkgRoot() / "ports" / package / "usage";
    if (usageFile.exists()) {
        Utils::FileReader reader;
        if (reader.fetch(usageFile))
            result = QString::fromUtf8(reader.data());
    } else {
        result = QString(
R"(The package %1 provides CMake targets:

    # this is heuristically generated, and may not be correct
    find_package(%1 CONFIG REQUIRED)
    target_link_libraries(main PRIVATE %1::%1))"
                     ).arg(package);
    }

    return result;
}

QString CMakeCodeDialog::cmakeCodeForPackages(const QStringList &packages)
{
    QString result;
    for (const QString &package : packages)
        result.append(cmakeCodeForPackage(package) + "\n\n");
    return result;
}

class VcpkgManifestEditorWidget : public TextEditor::TextEditorWidget
{
public:
    VcpkgManifestEditorWidget()
    {
        const QIcon vcpkgIcon = Utils::Icon({{":/vcpkg/images/vcpkgicon.png",
                                              Utils::Theme::IconsBaseColor}}).icon();
        m_searchPkgAction = toolBar()->addAction(vcpkgIcon, Tr::tr("Add vcpkg Package..."));
        connect(m_searchPkgAction, &QAction::triggered, this, [this] {
            const Search::VcpkgManifest package =
                Search::showVcpkgPackageSearchDialog(documentToManifest());
            if (!package.name.isEmpty()) {
                const QByteArray modifiedDocument =
                    addDependencyToManifest(textDocument()->contents(), package.name);
                textDocument()->setContents(modifiedDocument);
            }
        });

        const QIcon cmakeIcon = ProjectExplorer::Icons::CMAKE_LOGO_TOOLBAR.icon();
        m_cmakeCodeAction = toolBar()->addAction(cmakeIcon, Tr::tr("CMake Code..."));
        connect(m_cmakeCodeAction, &QAction::triggered, this, [this] {
            CMakeCodeDialog dlg(documentToManifest().dependencies);
            dlg.exec();
        });

        QAction *optionsAction = toolBar()->addAction(Utils::Icons::SETTINGS_TOOLBAR.icon(),
                                                      Core::ICore::msgShowOptionsDialog());
        connect(optionsAction, &QAction::triggered, [] {
            Core::ICore::showOptionsDialog(Constants::TOOLSSETTINGSPAGE_ID);
        });

        updateToolBar();
        connect(&settings().vcpkgRoot, &Utils::BaseAspect::changed,
                this, &VcpkgManifestEditorWidget::updateToolBar);
    }

    void updateToolBar()
    {
        Utils::FilePath vcpkg = settings().vcpkgRoot().pathAppended("vcpkg").withExecutableSuffix();
        const bool vcpkgEncabled = vcpkg.isExecutableFile();
        m_searchPkgAction->setEnabled(vcpkgEncabled);
        m_cmakeCodeAction->setEnabled(vcpkgEncabled);
    }

private:
    Search::VcpkgManifest documentToManifest() const
    {
        return Search::parseVcpkgManifest(textDocument()->contents());
    }

    QAction *m_searchPkgAction;
    QAction *m_cmakeCodeAction;
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

QByteArray addDependencyToManifest(const QByteArray &manifest, const QString &package)
{
    constexpr char dependenciesKey[] = "dependencies";
    QJsonObject jsonObject = QJsonDocument::fromJson(manifest).object();
    QJsonArray dependencies = jsonObject.value(dependenciesKey).toArray();
    dependencies.append(package);
    jsonObject.insert(dependenciesKey, dependencies);
    return QJsonDocument(jsonObject).toJson();
}

} // namespace Vcpkg::Internal
