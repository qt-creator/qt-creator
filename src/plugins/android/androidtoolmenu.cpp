// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "androidtoolmenu.h"

#include "androidconstants.h"
#include "androidmanifesteditor.h"
#include "androidtr.h"
#include "iconcontainerwidget.h"
#include "manifestwizard.h"
#include "permissionscontainerwidget.h"
#include "splashscreencontainerwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>

#include <QAction>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>

#include <texteditor/texteditor.h>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace Android::Internal {

static FilePath selectManifestDirectory(QWidget *parent, const FilePath &initialPath)
{
    QFileDialog dialog(parent, QObject::tr("Select Android Manifest Directory"));
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setOption(QFileDialog::DontResolveSymlinks, true);
    dialog.setDirectory(initialPath.toFSPathString());

    if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty())
        return FilePath::fromString(dialog.selectedFiles().at(0));

    return FilePath();
}

static FilePath getManifestDirWithWizardOption(QWidget *parent, const FilePath &initialPath)
{
        QMessageBox msgBox(parent);
        msgBox.setWindowTitle(QObject::tr("Select Android Manifest Directory"));
        msgBox.setText(QObject::tr("The Android manifest directory was not found automatically.\n"
                                   "Please select it manually or create it using the wizard."));
        msgBox.setIcon(QMessageBox::Question);

        QPushButton *selectBtn = msgBox.addButton(QObject::tr("Select Directory..."), QMessageBox::AcceptRole);
        QPushButton *wizardBtn = msgBox.addButton(QObject::tr("Create Android Templates..."), QMessageBox::ActionRole);
        msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();

        if (msgBox.clickedButton() == wizardBtn) {
            if (Project *project = ProjectManager::startupProject()) {
                if (BuildSystem *buildSys = project->activeTarget() ? project->activeTarget()->buildSystem() : nullptr) {
                    executeManifestWizard(buildSys);

                    FilePath androidDir = project->projectDirectory().pathAppended("android");
                    if (androidDir.pathAppended("AndroidManifest.xml").exists())
                        return androidDir;
                }
            }
            return FilePath();
        }

        if (msgBox.clickedButton() == selectBtn)
            return selectManifestDirectory(parent, initialPath);

        return FilePath();
}

static Project* getRelevantProject(const FilePath &docPath)
{
    if (Project *project = ProjectManager::projectForFile(docPath))
        return project;

    if (Project *startup = ProjectManager::startupProject())
        return startup;

    if (!ProjectManager::projects().isEmpty())
        return ProjectManager::projects().first();

    return nullptr;
}

FilePath manifestDir(TextEditor::TextEditorWidget *textEditorWidget)
{
    const FilePath docPath = textEditorWidget->textDocument()->filePath().absolutePath();
    Project *project = getRelevantProject(docPath);

    if (!project) {
        QMessageBox::warning(
            textEditorWidget,
            QObject::tr("Operation Failed"),
            QObject::tr("This operation requires a project to be open. Please open your project file first.")
            );
        return FilePath();
    }

    FilePath androidDir = project->projectDirectory().pathAppended("android");
    if (androidDir.pathAppended("AndroidManifest.xml").exists())
        return androidDir;

    return getManifestDirWithWizardOption(textEditorWidget, project->projectDirectory());
}

const char ANDROID_TOOLS_MENU_ID[] = "Android.Tools.Menu";
const char ANDROID_GRAPHICAL_EDITOR_ID[] = "Android.Tools.Graphical.Editor.ID";

class AndroidIconSplashEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AndroidIconSplashEditorWidget(QWidget *parent = nullptr);

    enum TabIndex {
        IconTab = 0,
        PermissionsTab = 1,
        SplashTab = 2
    };

    void setActiveTab(TabIndex index);

private:
    TextEditorWidget *m_textEditorWidget;
    QTabWidget *m_tabWidget;
};

void AndroidIconSplashEditorWidget::setActiveTab(TabIndex index)
{
    if (m_tabWidget)
        m_tabWidget->setCurrentIndex(index);
}

AndroidIconSplashEditorWidget::AndroidIconSplashEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto parentLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);

    auto manifestEditor = new AndroidManifestEditorWidget;
    TextEditor::TextEditorWidget *manifestTextEditor = manifestEditor->textEditorWidget();

    Utils::FilePath fallbackPath;
    if (Project *proj = ProjectManager::startupProject())
        fallbackPath = proj->projectDirectory();
    else if (!ProjectManager::projects().isEmpty())
        fallbackPath = ProjectManager::projects().first()->projectDirectory();
    else
        fallbackPath = Utils::FilePath::fromString(QDir::currentPath());
    manifestTextEditor->textDocument()->setFilePath(fallbackPath);

    auto permissionsContainer = new PermissionsContainerWidget(this);
    if (!permissionsContainer->initialize(manifestTextEditor))
        permissionsContainer->setEnabled(false);

    auto splashContainer = new SplashScreenContainerWidget(this);
    if (!splashContainer->initialize(manifestTextEditor))
        splashContainer->setEnabled(false);

    auto iconContainer = new IconContainerWidget(this);
    if (!iconContainer->initialize(manifestTextEditor))
        iconContainer->setEnabled(false);

    QScrollArea *permissionsScrollArea = new QScrollArea(this);
    permissionsScrollArea->setWidget(permissionsContainer);
    permissionsScrollArea->setWidgetResizable(true);
    permissionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    permissionsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QScrollArea *splashScrollArea = new QScrollArea(this);
    splashScrollArea->setWidget(splashContainer);
    splashScrollArea->setWidgetResizable(true);
    splashScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    splashScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QScrollArea *iconScrollArea = new QScrollArea(this);
    iconScrollArea->setWidget(iconContainer);
    iconScrollArea->setWidgetResizable(true);
    iconScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    iconScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_tabWidget->addTab(iconScrollArea, Tr::tr("Icon Editor"));
    m_tabWidget->addTab(permissionsScrollArea, Tr::tr("Permission Editor"));
    m_tabWidget->addTab(splashScrollArea, Tr::tr("Splash Screen Editor"));

    parentLayout->addWidget(m_tabWidget, 1);
}

class AndroidSplashscreenIconEditorFactory final : public Core::IEditorFactory
{
public:
    AndroidSplashscreenIconEditorFactory()
    {
        setId(ANDROID_GRAPHICAL_EDITOR_ID);
        setDisplayName(Android::Tr::tr("Android Manifest editor"));
        addMimeType(Android::Constants::ANDROID_MANIFEST_MIME_TYPE);
        setEditorCreator([]() -> Core::IEditor * {
            auto combinedWidget = new AndroidIconSplashEditorWidget();
            auto editor = new IconEditor(combinedWidget);
            return editor;
        });
    }
};

void setupAndroidToolsMenu()
{
    Core::MenuBuilder devMenu(ANDROID_TOOLS_MENU_ID);
    devMenu.setTitle(Android::Tr::tr("Android"));
    devMenu.addSeparator();
    devMenu.addToContainer(Core::Constants::M_TOOLS);

    static AndroidSplashscreenIconEditorFactory androidSplashscreenIconEditorFactoryInstance;
    const Utils::Id targetId(ANDROID_GRAPHICAL_EDITOR_ID);
    static QPointer<Core::IEditor> existingEditor;

    auto openEditorAtTab = [targetId](AndroidIconSplashEditorWidget::TabIndex tabIndex) {
        if (!existingEditor) {
            Core::IEditorFactory *factory = Core::IEditorFactory::editorFactoryForId(targetId);
            if (factory) {
                Core::IEditor *newEditor = factory->createEditor();
                if (newEditor) {
                    Core::EditorManager::addEditor(newEditor);
                    existingEditor = newEditor;
                }
            }
        }

        if (existingEditor) {
            Core::EditorManager::activateEditor(existingEditor);
            auto *activeWidget = qobject_cast<AndroidIconSplashEditorWidget*>(existingEditor->widget());
            if (activeWidget)
                activeWidget->setActiveTab(tabIndex);
        }
    };

    Core::ActionBuilder(nullptr, "Android.Tools.Icon")
        .setText(Android::Tr::tr("Icon Editor"))
        .addOnTriggered([openEditorAtTab]() {
            openEditorAtTab(AndroidIconSplashEditorWidget::IconTab);
        }).addToContainer(ANDROID_TOOLS_MENU_ID);

    Core::ActionBuilder(nullptr, "Android.Tools.Permissions")
        .setText(Android::Tr::tr("Permissions Editor"))
        .addOnTriggered([openEditorAtTab]() {
            openEditorAtTab(AndroidIconSplashEditorWidget::PermissionsTab);
        }).addToContainer(ANDROID_TOOLS_MENU_ID);

    Core::ActionBuilder(nullptr, "Android.Tools.Splashscreen")
        .setText(Android::Tr::tr("Splashscreen Editor"))
        .addOnTriggered([openEditorAtTab]() {
            openEditorAtTab(AndroidIconSplashEditorWidget::SplashTab);
        }).addToContainer(ANDROID_TOOLS_MENU_ID);

}

} // namespace Android::Internal

#include "androidtoolmenu.moc"
