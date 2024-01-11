// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "glsleditorplugin.h"

#include "glslcompletionassist.h"
#include "glsleditor.h"
#include "glsleditorconstants.h"
#include "glsleditortr.h"
#include "glslhighlighter.h"

#include <glsl/glslengine.h>
#include <glsl/glslparser.h>
#include <glsl/glsllexer.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/mimeconstants.h>

#include <QMenu>

using namespace Core;
using namespace Utils;

namespace GlslEditor::Internal {

class GlslEditorPluginPrivate
{
public:
    InitFile m_glsl_330_frag{"glsl_330.frag"};
    InitFile m_glsl_330_vert{"glsl_330.vert"};
    InitFile m_glsl_330_common{"glsl_330_common.glsl"};
    InitFile m_glsl_120_frag{"glsl_120.frag"};
    InitFile m_glsl_120_vert{"glsl_120.vert"};
    InitFile m_glsl_120_common{"glsl_120_common.glsl"};
    InitFile m_glsl_es_100_frag{"glsl_es_100.frag"};
    InitFile m_glsl_es_100_vert{"glsl_es_100.vert"};
    InitFile m_glsl_es_100_common{"glsl_es_100_common.glsl"};

    GlslCompletionAssistProvider completionAssistProvider;
};

static GlslEditorPluginPrivate *dd = nullptr;

InitFile::InitFile(const QString &fileName)
    : m_fileName(fileName)
{}

InitFile::~InitFile()
{
    delete m_engine;
}

void InitFile::initialize() const
{
    // Parse the builtins for any language variant so we can use all keywords.
    const int variant = GLSL::Lexer::Variant_All;

    QByteArray code;
    QFile file(ICore::resourcePath("glsl").pathAppended(m_fileName).toString());
    if (file.open(QFile::ReadOnly))
        code = file.readAll();

    m_engine = new GLSL::Engine();
    GLSL::Parser parser(m_engine, code.constData(), code.size(), variant);
    m_ast = parser.parse();
}

GLSL::TranslationUnitAST *InitFile::ast() const
{
    if (!m_ast)
        initialize();
    return m_ast;
}

GLSL::Engine *InitFile::engine() const
{
    if (!m_engine)
        initialize();
    return m_engine;
}

const InitFile *fragmentShaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_400)
        return &dd->m_glsl_330_frag;
    return (variant & GLSL::Lexer::Variant_GLSL_120)
            ? &dd->m_glsl_120_frag
            : &dd->m_glsl_es_100_frag;
}

const InitFile *vertexShaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_400)
        return &dd->m_glsl_330_vert;
    return (variant & GLSL::Lexer::Variant_GLSL_120)
            ? &dd->m_glsl_120_vert
            : &dd->m_glsl_es_100_vert;
}

const InitFile *shaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_400)
        return &dd->m_glsl_330_common;
    return (variant & GLSL::Lexer::Variant_GLSL_120)
            ? &dd->m_glsl_120_common
            : &dd->m_glsl_es_100_common;
}

class GlslEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GLSLEditor.json")

public:
    ~GlslEditorPlugin() final
    {
        delete dd;
        dd = nullptr;
    }

    void initialize() final
    {
        dd = new GlslEditorPluginPrivate;

        setupGlslEditorFactory();

        ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
        ActionContainer *glslToolsMenu = ActionManager::createMenu(Id(Constants::M_TOOLS_GLSL));
        glslToolsMenu->setOnAllDisabledBehavior(ActionContainer::Hide);
        QMenu *menu = glslToolsMenu->menu();
        //: GLSL sub-menu in the Tools menu
        menu->setTitle(Tr::tr("GLSL"));
        ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(glslToolsMenu);

        // Insert marker for "Refactoring" menu:
        Command *sep = contextMenu->addSeparator();
        sep->action()->setObjectName(Constants::M_REFACTORING_MENU_INSERTION_POINT);
        contextMenu->addSeparator();

        Command *cmd = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
        contextMenu->addAction(cmd);
    }

    void extensionsInitialized() final
    {
        using namespace Utils::Constants;
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_VERT_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_FRAG_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_ES_VERT_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png",
                                                         GLSL_ES_FRAG_MIMETYPE);
    }
};

} // GlslEditor::Internal

#include "glsleditorplugin.moc"
