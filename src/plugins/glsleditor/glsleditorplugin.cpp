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
#include <coreplugin/id.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/texteditorconstants.h>

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

namespace GlslEditor {
namespace Internal {

class GlslEditorPluginPrivate
{
public:
    GlslEditorPluginPrivate() :
        m_glsl_120_frag(0),
        m_glsl_120_vert(0),
        m_glsl_120_common(0),
        m_glsl_es_100_frag(0),
        m_glsl_es_100_vert(0),
        m_glsl_es_100_common(0)
    {}

    ~GlslEditorPluginPrivate()
    {
        delete m_glsl_120_frag;
        delete m_glsl_120_vert;
        delete m_glsl_120_common;
        delete m_glsl_es_100_frag;
        delete m_glsl_es_100_vert;
        delete m_glsl_es_100_common;
    }

    QPointer<BaseTextEditor> m_currentTextEditable;

    GlslEditorPlugin::InitFile *m_glsl_120_frag;
    GlslEditorPlugin::InitFile *m_glsl_120_vert;
    GlslEditorPlugin::InitFile *m_glsl_120_common;
    GlslEditorPlugin::InitFile *m_glsl_es_100_frag;
    GlslEditorPlugin::InitFile *m_glsl_es_100_vert;
    GlslEditorPlugin::InitFile *m_glsl_es_100_common;
};

static GlslEditorPluginPrivate *dd = 0;
static GlslEditorPlugin *m_instance = 0;

GlslEditorPlugin::InitFile::~InitFile()
{
    delete engine;
}

GlslEditorPlugin::GlslEditorPlugin()
{
    m_instance = this;
    dd = new GlslEditorPluginPrivate;
}

GlslEditorPlugin::~GlslEditorPlugin()
{
    delete dd;
    m_instance = 0;
}

bool GlslEditorPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    addAutoReleasedObject(new GlslEditorFactory);
    addAutoReleasedObject(new GlslCompletionAssistProvider);

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
    ActionContainer *glslToolsMenu = ActionManager::createMenu(Id(Constants::M_TOOLS_GLSL));
    glslToolsMenu->setOnAllDisabledBehavior(ActionContainer::Hide);
    QMenu *menu = glslToolsMenu->menu();
    //: GLSL sub-menu in the Tools menu
    menu->setTitle(tr("GLSL"));
    ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(glslToolsMenu);

    Command *cmd = 0;

    // Insert marker for "Refactoring" menu:
    Command *sep = contextMenu->addSeparator();
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
    contextMenu->addSeparator();

    cmd = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    errorMessage->clear();

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

ExtensionSystem::IPlugin::ShutdownFlag GlslEditorPlugin::aboutToShutdown()
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

static void parseGlslFile(const QString &fileName, GlslEditorPlugin::InitFile *initFile)
{
    // Parse the builtins for any langugage variant so we can use all keywords.
    const int variant = GLSL::Lexer::Variant_All;

    const QByteArray code = glslFile(fileName);
    initFile->engine = new GLSL::Engine();
    GLSL::Parser parser(initFile->engine, code.constData(), code.size(), variant);
    initFile->ast = parser.parse();
}

static GlslEditorPlugin::InitFile *getInitFile(const char *fileName, GlslEditorPlugin::InitFile **initFile)
{
    if (*initFile)
        return *initFile;
    *initFile = new GlslEditorPlugin::InitFile;
    parseGlslFile(QLatin1String(fileName), *initFile);
    return *initFile;
}

const GlslEditorPlugin::InitFile *GlslEditorPlugin::fragmentShaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return getInitFile("glsl_120.frag", &dd->m_glsl_120_frag);
    else
        return getInitFile("glsl_es_100.frag", &dd->m_glsl_es_100_frag);
}

const GlslEditorPlugin::InitFile *GlslEditorPlugin::vertexShaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return getInitFile("glsl_120.vert", &dd->m_glsl_120_vert);
    else
        return getInitFile("glsl_es_100.vert", &dd->m_glsl_es_100_vert);
}

const GlslEditorPlugin::InitFile *GlslEditorPlugin::shaderInit(int variant)
{
    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return getInitFile("glsl_120_common.glsl", &dd->m_glsl_120_common);
    else
        return getInitFile("glsl_es_100_common.glsl", &dd->m_glsl_es_100_common);
}

} // namespace Internal
} // namespace GlslEditor
