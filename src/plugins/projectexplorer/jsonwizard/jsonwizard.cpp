// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonwizard.h"

#include "jsonwizardfactory.h"
#include "jsonwizardgeneratorfactory.h"
#include "../project.h"
#include "../projectexplorer.h"
#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "../projecttree.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/itemviews.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/wizardpage.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QJSEngine>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

#ifdef WITH_TESTS
#include "jsonwizard_test.cpp"
#endif

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class ProjectFileTreeItem : public TreeItem
{
public:
    ProjectFileTreeItem(JsonWizard::GeneratorFile *candidate) : m_candidate(candidate)
    {
        toggleProjectFileStatus(false);
    }

    void toggleProjectFileStatus(bool on)
    {
        m_candidate->file.setAttributes(m_candidate->file.attributes()
                                        .setFlag(Core::GeneratedFile::OpenProjectAttribute, on));
    }

private:
    QVariant data(int column, int role) const override
    {
        if (column != 0 || role != Qt::DisplayRole)
            return {};
        return m_candidate->file.filePath().toUserOutput();
    }

    JsonWizard::GeneratorFile * const m_candidate;
};

class ProjectFilesModel : public TreeModel<TreeItem, ProjectFileTreeItem>
{
public:
    ProjectFilesModel(const QList<JsonWizard::GeneratorFile *> &candidates, QObject *parent)
        : TreeModel(parent)
    {
        setHeader({Tr::tr("Project File")});
        for (JsonWizard::GeneratorFile * const candidate : candidates)
            rootItem()->appendChild(new ProjectFileTreeItem(candidate));
    }
};

class ProjectFileChooser : public QDialog
{
public:
    ProjectFileChooser(const QList<JsonWizard::GeneratorFile *> &candidates, QWidget *parent)
        : QDialog(parent), m_view(new TreeView(this))
    {
        setWindowTitle(Tr::tr("Choose Project File"));
        const auto model = new ProjectFilesModel(candidates, this);
        m_view->setSelectionMode(TreeView::ExtendedSelection);
        m_view->setSelectionBehavior(TreeView::SelectRows);
        m_view->setModel(model);
        const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
        const auto updateOkButton = [buttonBox, this] {
            buttonBox->button(QDialogButtonBox::Ok)
                    ->setEnabled(m_view->selectionModel()->hasSelection());
        };
        connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, updateOkButton);
        updateOkButton();
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        const auto layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(Tr::tr(
            "The project contains more than one project file. "
            "Select the one you would like to use.")));
        layout->addWidget(m_view);
        layout->addWidget(buttonBox);
    }

private:
    void accept() override
    {
        const QModelIndexList selected = m_view->selectionModel()->selectedRows();
        const auto * const model = static_cast<ProjectFilesModel *>(m_view->model());
        for (const QModelIndex &index : selected) {
            const auto item = static_cast<ProjectFileTreeItem *>(model->itemForIndex(index));
            QTC_ASSERT(item, continue);
            item->toggleProjectFileStatus(true);
        }
        QDialog::accept();
    }

    TreeView * const m_view;
};

} // namespace Internal

JsonWizard::JsonWizard(QWidget *parent)
    : Wizard(parent)
{
    setMinimumSize(800, 500);
    m_expander.registerExtraResolver([this](const QString &name, QString *ret) -> bool {
        *ret = stringValue(name);
        return !ret->isNull();
    });
    m_expander.registerPrefix("Exists", Tr::tr("Check whether a variable exists.<br>"
                                           "Returns \"true\" if it does and an empty string if not."),
                   [this](const QString &value) -> QString
    {
        const QString key = QString::fromLatin1("%{") + value + QLatin1Char('}');
        return m_expander.expand(key) == key ? QString() : QLatin1String("true");
    });
    // override default JS macro by custom one that adds Wizard specific features
    m_jsExpander.registerObject("Wizard", new Internal::JsonWizardJsExtension(this));
    m_jsExpander.engine().evaluate("var value = Wizard.value");
    m_jsExpander.registerForExpander(&m_expander);
}

JsonWizard::~JsonWizard()
{
    qDeleteAll(m_generators);
}

void JsonWizard::addGenerator(JsonWizardGenerator *gen)
{
    QTC_ASSERT(gen, return);
    QTC_ASSERT(!m_generators.contains(gen), return);

    m_generators.append(gen);
}

MacroExpander *JsonWizard::expander()
{
    return &m_expander;
}

JsonWizard::GeneratorFiles JsonWizard::generateFileList()
{
    QString errorMessage;
    GeneratorFiles list;

    const FilePath targetPath =
            FilePath::fromString(stringValue(QLatin1String("TargetPath")));
    if (targetPath.isEmpty())
        errorMessage = Tr::tr("Could not determine target path. \"TargetPath\" was not set on any page.");

    if (m_files.isEmpty() && errorMessage.isEmpty()) {
        emit preGenerateFiles();
        for (JsonWizardGenerator *gen : std::as_const(m_generators)) {
            const FilePath wizardDir =
                    FilePath::fromString(stringValue(QLatin1String("WizardDir")));
            const Core::GeneratedFiles tmp =
                    gen->fileList(&m_expander, wizardDir, targetPath, &errorMessage);
            if (!errorMessage.isEmpty())
                break;
            list.append(Utils::transform(tmp, [&gen](const Core::GeneratedFile &f)
                                              { return JsonWizard::GeneratorFile(f, gen); }));
        }
    }

    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, Tr::tr("File Generation Failed"),
                              Tr::tr("The wizard failed to generate files.<br>"
                                 "The error message was: \"%1\".").arg(errorMessage));
        reject();
        return {};
    }

    QList<GeneratorFile *> projectFiles;
    for (JsonWizard::GeneratorFile &f : list) {
        if (f.file.attributes().testFlag(Core::GeneratedFile::OpenProjectAttribute))
            projectFiles << &f;
    }
    if (projectFiles.count() > 1)
        Internal::ProjectFileChooser(projectFiles, this).exec();

    return list;
}

void JsonWizard::commitToFileList(const JsonWizard::GeneratorFiles &list)
{
    m_files = list;
    emit postGenerateFiles(m_files);
}

QString JsonWizard::stringValue(const QString &n) const
{
    QVariant v = value(n);
    if (!v.isValid())
        return {};

    if (v.typeId() == QVariant::String) {
        QString tmp = m_expander.expand(v.toString());
        if (tmp.isEmpty())
            tmp = QString::fromLatin1(""); // Make sure isNull() is *not* true.
        return tmp;
    }

    if (v.typeId() == QVariant::StringList)
        return stringListToArrayString(v.toStringList(), &m_expander);

    return v.toString();
}

void JsonWizard::setValue(const QString &key, const QVariant &value)
{
    setProperty(key.toUtf8(), value);
}

QList<JsonWizard::OptionDefinition> JsonWizard::parseOptions(const QVariant &v, QString *errorMessage)
{
    QTC_ASSERT(errorMessage, return { });

    QList<JsonWizard::OptionDefinition> result;
    if (!v.isNull()) {
        const QVariantList optList = JsonWizardFactory::objectOrList(v, errorMessage);
        for (const QVariant &o : optList) {
            QVariantMap optionObject = o.toMap();
            JsonWizard::OptionDefinition odef;
            odef.m_key = optionObject.value(QLatin1String("key")).toString();
            odef.m_value = optionObject.value(QLatin1String("value")).toString();
            odef.m_condition = optionObject.value(QLatin1String("condition"), true);
            odef.m_evaluate = optionObject.value(QLatin1String("evaluate"), false);

            if (odef.m_key.isEmpty()) {
                *errorMessage = Tr::tr("No 'key' in options object.");
                result.clear();
                break;
            }
            result.append(odef);
        }
    }

    QTC_ASSERT(errorMessage->isEmpty() || (!errorMessage->isEmpty() && result.isEmpty()), return result);
    return result;
}

QVariant JsonWizard::value(const QString &n) const
{
    QVariant v = property(n.toUtf8());
    if (v.isValid())
        return v;
    if (hasField(n))
        return field(n); // Cannot contain macros!
    return {};
}

bool JsonWizard::boolFromVariant(const QVariant &v, MacroExpander *expander)
{
    if (v.typeId() == QVariant::String) {
        const QString tmp = expander->expand(v.toString());
        return !(tmp.isEmpty() || tmp == QLatin1String("false"));
    }
    return v.toBool();
}

QString JsonWizard::stringListToArrayString(const QStringList &list, const MacroExpander *expander)
{
    // Todo: Handle ' embedded in the strings better.
    if (list.isEmpty())
        return {};

    QStringList tmp = Utils::transform(list, [expander](const QString &i) {
        return expander->expand(i).replace(QLatin1Char('\''), QLatin1String("\\'"));
    });

    QString result;
    result.append(QLatin1Char('\''));
    result.append(tmp.join(QLatin1String("', '")));
    result.append(QLatin1Char('\''));

    return result;
}

void JsonWizard::removeAttributeFromAllFiles(Core::GeneratedFile::Attribute a)
{
    for (int i = 0; i < m_files.count(); ++i) {
        if (m_files.at(i).file.attributes() & a)
            m_files[i].file.setAttributes(m_files.at(i).file.attributes() ^ a);
    }
}

QHash<QString, QVariant> JsonWizard::variables() const
{
    QHash<QString, QVariant> result = Wizard::variables();
    const QList<QByteArray> properties = dynamicPropertyNames();
    for (const QByteArray &p : properties) {
        QString key = QString::fromUtf8(p);
        result.insert(key, value(key));
    }
    return result;
}

void JsonWizard::accept()
{
    auto page = qobject_cast<WizardPage *>(currentPage());
    if (page && page->handleAccept())
        return;

    Wizard::accept();

    QString errorMessage;
    if (m_files.isEmpty()) {
        commitToFileList(generateFileList()); // The Summary page does this for us, but a wizard
                                              // does not need to have one.
    }
    QTC_ASSERT(!m_files.isEmpty(), return);

    emit prePromptForOverwrite(m_files);
    JsonWizardGenerator::OverwriteResult overwrite =
            JsonWizardGenerator::promptForOverwrite(&m_files, &errorMessage);
    if (overwrite != JsonWizardGenerator::OverwriteOk) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, Tr::tr("Failed to Overwrite Files"), errorMessage);
        return;
    }

    emit preFormatFiles(m_files);
    if (!JsonWizardGenerator::formatFiles(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, Tr::tr("Failed to Format Files"), errorMessage);
        return;
    }

    const QList<Core::IDocument *> documentsToClose
        = transform(m_files, [](const GeneratorFile &file) -> Core::IDocument * {
              if ((file.file.attributes() & Core::GeneratedFile::OpenEditorAttribute) == 0)
                  return nullptr;
              return Core::DocumentModel::documentForFilePath(file.file.filePath());
          });
    Core::EditorManager::closeDocuments(documentsToClose, /*askAboutModifiedEditors=*/false);

    emit preWriteFiles(m_files);
    if (!JsonWizardGenerator::writeFiles(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, Tr::tr("Failed to Write Files"), errorMessage);
        return;
    }

    emit postProcessFiles(m_files);
    if (!JsonWizardGenerator::postWrite(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, Tr::tr("Failed to Post-Process Files"), errorMessage);
        return;
    }
    emit filesReady(m_files);
    if (!JsonWizardGenerator::polish(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, Tr::tr("Failed to Polish Files"), errorMessage);
        return;
    }
    emit filesPolished(m_files);
    if (!JsonWizardGenerator::allDone(this, &m_files, &errorMessage)) {
        if (!errorMessage.isEmpty())
            QMessageBox::warning(this, Tr::tr("Failed to Open Files"), errorMessage);
        return;
    }
    emit allDone(m_files);

    openFiles(m_files);

    auto node = static_cast<ProjectExplorer::Node*>(value(ProjectExplorer::Constants::PREFERRED_PROJECT_NODE).value<void*>());
    if (node && ProjectTree::hasNode(node)) // PREFERRED_PROJECT_NODE is not set for newly created projects
        openProjectForNode(node);
}

void JsonWizard::reject()
{
    auto page = qobject_cast<WizardPage *>(currentPage());
    if (page && page->handleReject())
        return;

    Wizard::reject();
}

void JsonWizard::handleNewPages(int pageId)
{
    auto wp = qobject_cast<WizardPage *>(page(pageId));
    if (!wp)
        return;

    connect(wp, &WizardPage::reportError, this, &JsonWizard::handleError);
}

void JsonWizard::handleError(const QString &message)
{
    Core::MessageManager::writeDisrupting(message);
}

QString JsonWizard::stringify(const QVariant &v) const
{
    if (v.typeId() == QVariant::StringList)
        return stringListToArrayString(v.toStringList(), &m_expander);
    return Wizard::stringify(v);
}

QString JsonWizard::evaluate(const QVariant &v) const
{
    return m_expander.expand(stringify(v));
}

void JsonWizard::openFiles(const JsonWizard::GeneratorFiles &files)
{
    QString errorMessage;
    bool openedSomething = stringValue("DoNotOpenFile") == "true";
    static const auto formatFile = [](Core::IEditor *editor) {
        editor->document()->formatContents();
        editor->document()->save(nullptr);
    };
    for (const JsonWizard::GeneratorFile &f : files) {
        const Core::GeneratedFile &file = f.file;
        if (!file.filePath().exists()) {
            errorMessage = Tr::tr("\"%1\" does not exist in the file system.")
                    .arg(file.filePath().toUserOutput());
            break;
        }
        if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            ProjectExplorerPlugin::OpenProjectResult result
                    = ProjectExplorerPlugin::openProject(file.filePath());
            if (!result) {
                errorMessage = result.errorMessage();
                if (errorMessage.isEmpty()) {
                    errorMessage = Tr::tr("Failed to open \"%1\" as a project.")
                            .arg(file.filePath().toUserOutput());
                }
                break;
            }
            result.project()->setNeedsInitialExpansion(true);
            openedSomething = true;
        }
        if (file.attributes() & Core::GeneratedFile::OpenEditorAttribute) {
            Core::IEditor *editor = Core::EditorManager::openEditor(file.filePath(),
                                                                    file.editorId());
            if (!editor) {
                errorMessage = Tr::tr("Failed to open an editor for \"%1\".")
                        .arg(file.filePath().toUserOutput());
                break;
            } else if (file.attributes() & Core::GeneratedFile::TemporaryFile) {
                editor->document()->setTemporary(true);
            } else {
                formatFile(editor);
            }
            openedSomething = true;
        } else if (file.filePath().fileSize() < 100 * 1024 ) {
            Core::EditorManager::runWithTemporaryEditor(file.filePath(), formatFile);
        }
    }

    const QString path
            = QDir::toNativeSeparators(m_expander.expand(value(QLatin1String("TargetPath")).toString()));

    // Now try to find the project file and open
    if (!openedSomething) {
        errorMessage = Tr::tr("No file to open found in \"%1\".").arg(path);
    }

    if (!errorMessage.isEmpty()) {
        const QString text = path.isEmpty() ? Tr::tr("Failed to open project.")
                                            : Tr::tr("Failed to open project in \"%1\".").arg(path);
        QMessageBox msgBox(QMessageBox::Warning, Tr::tr("Cannot Open Project"), text);
        msgBox.setDetailedText(errorMessage);
        msgBox.addButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void JsonWizard::openProjectForNode(Node *node)
{
    const ProjectNode *projNode = node->asProjectNode();
    if (!projNode) {
        if (ContainerNode * const cn = node->asContainerNode())
            projNode = cn->rootProjectNode();
        else
            projNode = node->parentProjectNode();
    }
    QTC_ASSERT(projNode, return);

    std::optional<FilePath> projFilePath = projNode->visibleAfterAddFileAction();

    if (projFilePath && !Core::EditorManager::openEditor(projFilePath.value())) {
            auto errorMessage = Tr::tr("Failed to open an editor for \"%1\".")
                    .arg(QDir::toNativeSeparators(projFilePath.value().toString()));
            QMessageBox::warning(nullptr, Tr::tr("Cannot Open Project"), errorMessage);
    }
}

QString JsonWizard::OptionDefinition::value(MacroExpander &expander) const
{
    if (JsonWizard::boolFromVariant(m_evaluate, &expander))
        return expander.expand(m_value);
    return m_value;
}

bool JsonWizard::OptionDefinition::condition(MacroExpander &expander) const
{
    return JsonWizard::boolFromVariant(m_condition, &expander);
}

namespace Internal {

JsonWizardJsExtension::JsonWizardJsExtension(JsonWizard *wizard)
    : m_wizard(wizard)
{}

QVariant JsonWizardJsExtension::value(const QString &name) const
{
    return m_wizard->expander()->expandVariant(m_wizard->value(name));
}

} // namespace Internal
} // namespace ProjectExplorer
