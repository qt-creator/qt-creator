/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "glsleditorplugin.h"
#include "glslcompletionassist.h"
#include "glsleditor.h"
#include "glsleditorconstants.h"
#include "glslfilewizard.h"
#include "glslhighlighter.h"
#include "glslhoverhandler.h"

#include <glsl/glslengine.h>
#include <glsl/glslparser.h>
#include <glsl/glsllexer.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/mimedatabase.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/highlighterfactory.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/textfilewizard.h>

#include <utils/qtcassert.h>

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QMenu>
#include <QSettings>
#include <QTimer>
#include <QtPlugin>

using namespace Core;
using namespace TextEditor;

namespace GLSLEditor {
namespace Internal {

class GLSLEditorPluginPrivate
{
public:
    GLSLEditorPluginPrivate() :
        m_glsl_120_frag(0),
        m_glsl_120_vert(0),
        m_glsl_120_common(0),
        m_glsl_es_100_frag(0),
        m_glsl_es_100_vert(0),
        m_glsl_es_100_common(0)
    {}

    ~GLSLEditorPluginPrivate()
    {
        delete m_glsl_120_frag;
        delete m_glsl_120_vert;
        delete m_glsl_120_common;
        delete m_glsl_es_100_frag;
        delete m_glsl_es_100_vert;
        delete m_glsl_es_100_common;
    }

    QPointer<TextEditor::BaseTextEditor> m_currentTextEditable;

    GLSLEditorPlugin::InitFile *m_glsl_120_frag;
    GLSLEditorPlugin::InitFile *m_glsl_120_vert;
    GLSLEditorPlugin::InitFile *m_glsl_120_common;
    GLSLEditorPlugin::InitFile *m_glsl_es_100_frag;
    GLSLEditorPlugin::InitFile *m_glsl_es_100_vert;
    GLSLEditorPlugin::InitFile *m_glsl_es_100_common;
};

static GLSLEditorPluginPrivate *dd = 0;
static GLSLEditorPlugin *m_instance = 0;

GLSLEditorPlugin::InitFile::~InitFile()
{
    delete engine;
}

GLSLEditorPlugin::GLSLEditorPlugin()
{
    m_instance = this;
    dd = new GLSLEditorPluginPrivate;
}

GLSLEditorPlugin::~GLSLEditorPlugin()
{
    delete dd;
    m_instance = 0;
}

bool GLSLEditorPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    if (!MimeDatabase::addMimeTypes(QLatin1String(":/glsleditor/GLSLEditor.mimetypes.xml"), errorMessage))
        return false;

    addAutoReleasedObject(new GLSLHoverHandler(this));
    addAutoReleasedObject(new GlslEditorFactory);
    addAutoReleasedObject(new GLSLCompletionAssistProvider);

    ActionContainer *contextMenu = ActionManager::createMenu(GLSLEditor::Constants::M_CONTEXT);
    ActionContainer *glslToolsMenu = ActionManager::createMenu(Id(Constants::M_TOOLS_GLSL));
    glslToolsMenu->setOnAllDisabledBehavior(ActionContainer::Hide);
    QMenu *menu = glslToolsMenu->menu();
    //: GLSL sub-menu in the Tools menu
    menu->setTitle(tr("GLSL"));
    ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(glslToolsMenu);

    Command *cmd = 0;

    // Insert marker for "Refactoring" menu:
    Context globalContext(Core::Constants::C_GLOBAL);
    Command *sep = contextMenu->addSeparator(globalContext);
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
    contextMenu->addSeparator(globalContext);

    cmd = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    errorMessage->clear();

    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE);
    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE_VERT);
    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE_FRAG);
    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE_VERT_ES);
    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE_FRAG_ES);

    IWizardFactory *wizard = new GLSLFileWizard(GLSLFileWizard::FragmentShaderES);
    wizard->setWizardKind(IWizardFactory::FileWizard);
    wizard->setCategory(QLatin1String(Constants::WIZARD_CATEGORY_GLSL));
    wizard->setDisplayCategory(QCoreApplication::translate("GLSLEditor", Constants::WIZARD_TR_CATEGORY_GLSL));
    wizard->setDescription
        (tr("Creates a fragment shader in the OpenGL/ES 2.0 Shading "
            "Language (GLSL/ES). Fragment shaders generate the final "
            "pixel colors for triangles, points and lines rendered "
            "with OpenGL."));
    wizard->setDisplayName(tr("Fragment Shader (OpenGL/ES 2.0)"));
    wizard->setId(QLatin1String("F.GLSL"));
    addAutoReleasedObject(wizard);

    wizard = new GLSLFileWizard(GLSLFileWizard::VertexShaderES);
    wizard->setWizardKind(IWizardFactory::FileWizard);
    wizard->setCategory(QLatin1String(Constants::WIZARD_CATEGORY_GLSL));
    wizard->setDisplayCategory(QCoreApplication::translate("GLSLEditor", Constants::WIZARD_TR_CATEGORY_GLSL));
    wizard->setDescription
        (tr("Creates a vertex shader in the OpenGL/ES 2.0 Shading "
            "Language (GLSL/ES). Vertex shaders transform the "
            "positions, normals and texture co-ordinates of "
            "triangles, points and lines rendered with OpenGL."));
    wizard->setDisplayName(tr("Vertex Shader (OpenGL/ES 2.0)"));
    wizard->setId(QLatin1String("G.GLSL"));
    addAutoReleasedObject(wizard);

    wizard = new GLSLFileWizard(GLSLFileWizard::FragmentShaderDesktop);
    wizard->setWizardKind(IWizardFactory::FileWizard);
    wizard->setCategory(QLatin1String(Constants::WIZARD_CATEGORY_GLSL));
    wizard->setDisplayCategory(QCoreApplication::translate("GLSLEditor", Constants::WIZARD_TR_CATEGORY_GLSL));
    wizard->setDescription
        (tr("Creates a fragment shader in the Desktop OpenGL Shading "
            "Language (GLSL). Fragment shaders generate the final "
            "pixel colors for triangles, points and lines rendered "
            "with OpenGL."));
    wizard->setDisplayName(tr("Fragment Shader (Desktop OpenGL)"));
    wizard->setId(QLatin1String("J.GLSL"));
    addAutoReleasedObject(wizard);

    wizard = new GLSLFileWizard(GLSLFileWizard::VertexShaderDesktop);
    wizard->setWizardKind(IWizardFactory::FileWizard);
    wizard->setCategory(QLatin1String(Constants::WIZARD_CATEGORY_GLSL));
    wizard->setDisplayCategory(QCoreApplication::translate("GLSLEditor", Constants::WIZARD_TR_CATEGORY_GLSL));
    wizard->setDescription
        (tr("Creates a vertex shader in the Desktop OpenGL Shading "
            "Language (GLSL). Vertex shaders transform the "
            "positions, normals and texture co-ordinates of "
            "triangles, points and lines rendered with OpenGL."));
    wizard->setDisplayName(tr("Vertex Shader (Desktop OpenGL)"));
    wizard->setId(QLatin1String("K.GLSL"));
    addAutoReleasedObject(wizard);

    auto hf = new TextEditor::HighlighterFactory;
    hf->setProductType<Highlighter>();
    hf->setId(GLSLEditor::Constants::C_GLSLEDITOR_ID);
    hf->addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE);
    hf->addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE_VERT);
    hf->addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG);
    hf->addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE_VERT_ES);
    hf->addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG_ES);
    addAutoReleasedObject(hf);

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

static QByteArray glslFile(const QString &fileName)
{
    QFile file(ICore::resourcePath() + QLatin1String("/glsl/") + fileName);
    if (file.open(QFile::ReadOnly))
        return file.readAll();
    return QByteArray();
}

static void parseGlslFile(const QString &fileName, GLSLEditorPlugin::InitFile *initFile)
{
    // Parse the builtins for any langugage variant so we can use all keywords.
    const int variant = GLSL::Lexer::Variant_All;

    const QByteArray code = glslFile(fileName);
    initFile->engine = new GLSL::Engine();
    GLSL::Parser parser(initFile->engine, code.constData(), code.size(), variant);
    initFile->ast = parser.parse();
}

static GLSLEditorPlugin::InitFile *getInitFile(const char *fileName, GLSLEditorPlugin::InitFile **initFile)
{
    if (*initFile)
        return *initFile;
    *initFile = new GLSLEditorPlugin::InitFile;
    parseGlslFile(QLatin1String(fileName), *initFile);
    return *initFile;
}

const GLSLEditorPlugin::InitFile *GLSLEditorPlugin::fragmentShaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return getInitFile("glsl_120.frag", &dd->m_glsl_120_frag);
    else
        return getInitFile("glsl_es_100.frag", &dd->m_glsl_es_100_frag);
}

const GLSLEditorPlugin::InitFile *GLSLEditorPlugin::vertexShaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return getInitFile("glsl_120.vert", &dd->m_glsl_120_vert);
    else
        return getInitFile("glsl_es_100.vert", &dd->m_glsl_es_100_vert);
}

const GLSLEditorPlugin::InitFile *GLSLEditorPlugin::shaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return getInitFile("glsl_120_common.glsl", &dd->m_glsl_120_common);
    else
        return getInitFile("glsl_es_100_common.glsl", &dd->m_glsl_es_100_common);
}

} // namespace Internal
} // namespace GLSLEditor

Q_EXPORT_PLUGIN(GLSLEditor::Internal::GLSLEditorPlugin)
