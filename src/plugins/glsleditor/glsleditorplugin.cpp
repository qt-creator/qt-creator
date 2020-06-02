/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "glsleditorplugin.h"
#include "glslcompletionassist.h"
#include "glsleditor.h"
#include "glsleditorconstants.h"
#include "glslhighlighter.h"

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

#include <QMenu>

using namespace Core;
using namespace Utils;

namespace GlslEditor {
namespace Internal {

class GlslEditorPluginPrivate
{
public:
    GlslEditorPlugin::InitFile m_glsl_330_frag{"glsl_330.frag"};
    GlslEditorPlugin::InitFile m_glsl_330_vert{"glsl_330.vert"};
    GlslEditorPlugin::InitFile m_glsl_330_common{"glsl_330_common.glsl"};
    GlslEditorPlugin::InitFile m_glsl_120_frag{"glsl_120.frag"};
    GlslEditorPlugin::InitFile m_glsl_120_vert{"glsl_120.vert"};
    GlslEditorPlugin::InitFile m_glsl_120_common{"glsl_120_common.glsl"};
    GlslEditorPlugin::InitFile m_glsl_es_100_frag{"glsl_es_100.frag"};
    GlslEditorPlugin::InitFile m_glsl_es_100_vert{"glsl_es_100.vert"};
    GlslEditorPlugin::InitFile m_glsl_es_100_common{"glsl_es_100_common.glsl"};

    GlslEditorFactory editorFactory;
    GlslCompletionAssistProvider completionAssistProvider;
};

static GlslEditorPluginPrivate *dd = nullptr;

GlslEditorPlugin::InitFile::InitFile(const QString &fileName)
    : m_fileName(fileName)
{}


GlslEditorPlugin::InitFile::~InitFile()
{
    delete m_engine;
}

void GlslEditorPlugin::InitFile::initialize() const
{
    // Parse the builtins for any language variant so we can use all keywords.
    const int variant = GLSL::Lexer::Variant_All;

    QByteArray code;
    QFile file(ICore::resourcePath() + "/glsl/" + m_fileName);
    if (file.open(QFile::ReadOnly))
        code = file.readAll();

    m_engine = new GLSL::Engine();
    GLSL::Parser parser(m_engine, code.constData(), code.size(), variant);
    m_ast = parser.parse();
}

GLSL::TranslationUnitAST *GlslEditorPlugin::InitFile::ast() const
{
    if (!m_ast)
        initialize();
    return m_ast;
}

GLSL::Engine *GlslEditorPlugin::InitFile::engine() const
{
    if (!m_engine)
        initialize();
    return m_engine;
}

GlslEditorPlugin::~GlslEditorPlugin()
{
    delete dd;
    dd = nullptr;
}

bool GlslEditorPlugin::initialize(const QStringList &, QString *)
{
    dd = new GlslEditorPluginPrivate;

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
    ActionContainer *glslToolsMenu = ActionManager::createMenu(Id(Constants::M_TOOLS_GLSL));
    glslToolsMenu->setOnAllDisabledBehavior(ActionContainer::Hide);
    QMenu *menu = glslToolsMenu->menu();
    //: GLSL sub-menu in the Tools menu
    menu->setTitle(tr("GLSL"));
    ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(glslToolsMenu);

    // Insert marker for "Refactoring" menu:
    Command *sep = contextMenu->addSeparator();
    sep->action()->setObjectName(Constants::M_REFACTORING_MENU_INSERTION_POINT);
    contextMenu->addSeparator();

    Command *cmd = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    return true;
}

void GlslEditorPlugin::extensionsInitialized()
{
    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE);
    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE_VERT);
    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE_FRAG);
    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE_VERT_ES);
    FileIconProvider::registerIconOverlayForMimeType(":/glsleditor/images/glslfile.png", Constants::GLSL_MIMETYPE_FRAG_ES);
}

const GlslEditorPlugin::InitFile *GlslEditorPlugin::fragmentShaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_400)
        return &dd->m_glsl_330_frag;
    return (variant & GLSL::Lexer::Variant_GLSL_120)
            ? &dd->m_glsl_120_frag
            : &dd->m_glsl_es_100_frag;
}

const GlslEditorPlugin::InitFile *GlslEditorPlugin::vertexShaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_400)
        return &dd->m_glsl_330_vert;
    return (variant & GLSL::Lexer::Variant_GLSL_120)
            ? &dd->m_glsl_120_vert
            : &dd->m_glsl_es_100_vert;
}

const GlslEditorPlugin::InitFile *GlslEditorPlugin::shaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_400)
        return &dd->m_glsl_330_common;
    return (variant & GLSL::Lexer::Variant_GLSL_120)
            ? &dd->m_glsl_120_common
            : &dd->m_glsl_es_100_common;
}

} // namespace Internal
} // namespace GlslEditor
