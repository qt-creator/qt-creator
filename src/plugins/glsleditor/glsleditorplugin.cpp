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

#include "glsleditorplugin.h"
#include "glsleditor.h"
#include "glsleditorconstants.h"
#include "glsleditorfactory.h"
#include "glslcodecompletion.h"
#include "glslfilewizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/taskhub.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textfilewizard.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/completionsupport.h>
#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QAction>

using namespace GLSLEditor;
using namespace GLSLEditor::Internal;
using namespace GLSLEditor::Constants;

GLSLEditorPlugin *GLSLEditorPlugin::m_instance = 0;

GLSLEditorPlugin::GLSLEditorPlugin() :
    m_editor(0),
    m_actionHandler(0)
{
    m_instance = this;
}

GLSLEditorPlugin::~GLSLEditorPlugin()
{
    removeObject(m_editor);
    delete m_actionHandler;
    m_instance = 0;
}

/*! Copied from cppplugin.cpp */
static inline
Core::Command *createSeparator(Core::ActionManager *am,
                               QObject *parent,
                               Core::Context &context,
                               const char *id)
{
    QAction *separator = new QAction(parent);
    separator->setSeparator(true);
    return am->registerAction(separator, Core::Id(id), context);
}

bool GLSLEditorPlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/glsleditor/GLSLEditor.mimetypes.xml"), error_message))
        return false;

//    m_modelManager = new ModelManager(this);
//    addAutoReleasedObject(m_modelManager);

    Core::Context context(GLSLEditor::Constants::C_GLSLEDITOR_ID);

    m_editor = new GLSLEditorFactory(this);
    addObject(m_editor);

    CodeCompletion *completion = new CodeCompletion(this);
    addAutoReleasedObject(completion);

    m_actionHandler = new TextEditor::TextEditorActionHandler(GLSLEditor::Constants::C_GLSLEDITOR_ID,
                                                              TextEditor::TextEditorActionHandler::Format
                                                              | TextEditor::TextEditorActionHandler::UnCommentSelection
                                                              | TextEditor::TextEditorActionHandler::UnCollapseAll);
    m_actionHandler->initializeActions();

    Core::ActionManager *am =  core->actionManager();
    Core::ActionContainer *contextMenu = am->createMenu(GLSLEditor::Constants::M_CONTEXT);
    Core::ActionContainer *glslToolsMenu = am->createMenu(Core::Id(Constants::M_TOOLS_GLSL));
    glslToolsMenu->setEmptyAction(Core::ActionContainer::EA_Hide);
    QMenu *menu = glslToolsMenu->menu();
    //: GLSL sub-menu in the Tools menu
    menu->setTitle(tr("GLSL"));
    am->actionContainer(Core::Constants::M_TOOLS)->addMenu(glslToolsMenu);

    Core::Command *cmd = 0;

    // Insert marker for "Refactoring" menu:
    Core::Context globalContext(Core::Constants::C_GLOBAL);
    Core::Command *sep = createSeparator(am, this, globalContext,
                                         Constants::SEPARATOR1);
    sep->action()->setObjectName(Constants::M_REFACTORING_MENU_INSERTION_POINT);
    contextMenu->addAction(sep);
    contextMenu->addAction(createSeparator(am, this, globalContext,
                                           Constants::SEPARATOR2));

    cmd = am->command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    // Set completion settings and keep them up to date
    TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
    completion->setCompletionSettings(textEditorSettings->completionSettings());
    connect(textEditorSettings, SIGNAL(completionSettingsChanged(TextEditor::CompletionSettings)),
            completion, SLOT(setCompletionSettings(TextEditor::CompletionSettings)));

    error_message->clear();

    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(":/glsleditor/images/glslfile.png")), "glsl");

    Core::BaseFileWizardParameters fragWizardParameters(Core::IWizard::FileWizard);
    fragWizardParameters.setCategory(QLatin1String(Constants::WIZARD_CATEGORY_GLSL));
    fragWizardParameters.setDisplayCategory(QCoreApplication::translate("GLSLEditor", Constants::WIZARD_TR_CATEGORY_GLSL));
    fragWizardParameters.setDescription
        (tr("Creates a fragment shader in the OpenGL/ES 2.0 Shading "
            "Language (GLSL/ES).  Fragment shaders generate the final "
            "pixel colors for triangles, points, and lines rendered "
            "with OpenGL."));
    fragWizardParameters.setDisplayName(tr("Fragment shader"));
    fragWizardParameters.setId(QLatin1String("F.GLSL"));
    addAutoReleasedObject(new GLSLFileWizard(fragWizardParameters, GLSLFileWizard::FragmentShader, core));

    Core::BaseFileWizardParameters vertWizardParameters(Core::IWizard::FileWizard);
    vertWizardParameters.setCategory(QLatin1String(Constants::WIZARD_CATEGORY_GLSL));
    vertWizardParameters.setDisplayCategory(QCoreApplication::translate("GLSLEditor", Constants::WIZARD_TR_CATEGORY_GLSL));
    vertWizardParameters.setDescription
        (tr("Creates a vertex shader in the OpenGL/ES 2.0 Shading "
            "Language (GLSL/ES).  Vertex shaders transform the "
            "positions, normals, and texture co-ordinates of "
            "triangles, points, and lines rendered with OpenGL."));
    vertWizardParameters.setDisplayName(tr("Vertex shader"));
    vertWizardParameters.setId(QLatin1String("V.GLSL"));
    addAutoReleasedObject(new GLSLFileWizard(vertWizardParameters, GLSLFileWizard::VertexShader, core));

    return true;
}

void GLSLEditorPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag GLSLEditorPlugin::aboutToShutdown()
{
    // delete GLSL::Icons::instance(); // delete object held by singleton

    return IPlugin::aboutToShutdown();
}

void GLSLEditorPlugin::initializeEditor(GLSLEditor::GLSLTextEditor *editor)
{
    QTC_ASSERT(m_instance, /**/);

    m_actionHandler->setupActions(editor);

    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);

//    // auto completion
    connect(editor, SIGNAL(requestAutoCompletion(TextEditor::ITextEditable*, bool)),
            TextEditor::CompletionSupport::instance(), SLOT(autoComplete(TextEditor::ITextEditable*, bool)));

//    // quick fix
//    connect(editor, SIGNAL(requestQuickFix(TextEditor::ITextEditable*)),
//            this, SLOT(quickFix(TextEditor::ITextEditable*)));
}


Core::Command *GLSLEditorPlugin::addToolAction(QAction *a, Core::ActionManager *am,
                                               Core::Context &context, const QString &name,
                                               Core::ActionContainer *c1, const QString &keySequence)
{
    Core::Command *command = am->registerAction(a, name, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    c1->addAction(command);
    return command;
}

Q_EXPORT_PLUGIN(GLSLEditorPlugin)
