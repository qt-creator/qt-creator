// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "androidtoolmenu.h"

#include "androidmanifesteditor.h"
#include "androidconstants.h"
#include "androidtr.h"
#include "iconcontainerwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>

#include <QMenu>
#include <QAction>
#include <QCoreApplication>

#include <texteditor/texteditor.h>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace Android::Internal {

const char ANDROID_TOOLS_MENU_ID[] = "Android.Tools.Menu";
const char ANDROID_GRAPHICAL_EDITOR_ID[] = "Android.Tools.Graphical.Editor.ID";

class AndroidIconSplashEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AndroidIconSplashEditorWidget(QWidget *parent = nullptr);

    enum TabIndex {
        SplashTab = 0,
        IconTab = 1
    };

    void setActiveTab(TabIndex index);

private:
    TextEditorWidget *m_textEditorWidget;
    QTabWidget *m_tabWidget;
};

void AndroidIconSplashEditorWidget::setActiveTab(TabIndex index) {
    if (m_tabWidget)
        m_tabWidget->setCurrentIndex(index);
}

AndroidIconSplashEditorWidget::AndroidIconSplashEditorWidget(QWidget *parent) : QWidget(parent)
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

    auto iconContainer = new QWidget(this);
    auto splashContainer = new QWidget(this);
    auto permissionsContainer = new QWidget(this);

    m_tabWidget->addTab(splashContainer, Tr::tr("Splash Screen Editor"));
    m_tabWidget->addTab(iconContainer, Tr::tr("Icon Editor"));

    permissionsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    parentLayout->addWidget(permissionsContainer, 1);
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

    Core::ActionBuilder(nullptr, "Android.Tools.Splashscreen")
        .setText(Android::Tr::tr("Splashscreen Editor"))
        .addOnTriggered([openEditorAtTab]() {
            openEditorAtTab(AndroidIconSplashEditorWidget::SplashTab);
        }).addToContainer(ANDROID_TOOLS_MENU_ID);

    Core::ActionBuilder(nullptr, "Android.Tools.Permissions")
        .setText(Android::Tr::tr("Permissions Editor"))
        .addOnTriggered([openEditorAtTab]() {
            openEditorAtTab(AndroidIconSplashEditorWidget::SplashTab);
        }).addToContainer(ANDROID_TOOLS_MENU_ID);
}

} //Android::Internal

#include "androidtoolmenu.moc"

