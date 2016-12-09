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

#include "profilecompletionassist.h"
#include "qmakeprojectmanagerconstants.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/keywordscompletionassist.h>
#include <texteditor/texteditorconstants.h>

#include <coreplugin/id.h>

using namespace QmakeProjectManager::Internal;
using namespace TextEditor;


static const char *const variableKeywords[] = {
    "CCFLAG",
    "CLEAN_DEPS",
    "CONFIG",
    "DEFINES",
    "DEF_FILE",
    "DEPENDPATH",
    "DEPLOYMENT",
    "DEPLOYMENT_PLUGIN",
    "DESTDIR",
    "DESTDIR_TARGET",
    "DISTFILES",
    "DLLDESTDIR",
    "DSP_TEMPLATE",
    "FORMS",
    "FORMS3",
    "GUID",
    "HEADERS",
    "ICON",
    "INCLUDEPATH",
    "INSTALLS",
    "LEXIMPLS",
    "LEXOBJECTS",
    "LEXSOURCES",
    "LIBS",
    "LITERAL_HASH",
    "MAKEFILE",
    "MAKEFILE_GENERATOR",
    "MOBILITY",
    "MOC_DIR",
    "OBJECTIVE_HEADERS",
    "OBJECTIVE_SOURCES",
    "OBJECTS",
    "OBJECTS_DIR",
    "OBJMOC",
    "OTHER_FILES",
    "OUT_PWD",
    "PKGCONFIG",
    "POST_TARGETDEPS",
    "PRECOMPILED_HEADER",
    "PRE_TARGETDEPS",
    "PWD",
    "QMAKE",
    "QMAKESPEC",
    "QMAKE_APP_FLAG",
    "QMAKE_APP_OR_DLL",
    "QMAKE_AR_CMD",
    "QMAKE_BUNDLE_DATA",
    "QMAKE_BUNDLE_EXTENSION",
    "QMAKE_CC",
    "QMAKE_CFLAGS",
    "QMAKE_CFLAGS_DEBUG",
    "QMAKE_CFLAGS_MT",
    "QMAKE_CFLAGS_MT_DBG",
    "QMAKE_CFLAGS_MT_DLL",
    "QMAKE_CFLAGS_MT_DLLDBG",
    "QMAKE_CFLAGS_RELEASE",
    "QMAKE_CFLAGS_SHLIB",
    "QMAKE_CFLAGS_THREAD",
    "QMAKE_CFLAGS_WARN_OFF",
    "QMAKE_CFLAGS_WARN_ON",
    "QMAKE_CLEAN",
    "QMAKE_CXX",
    "QMAKE_CXXFLAGS",
    "QMAKE_CXXFLAGS_DEBUG",
    "QMAKE_CXXFLAGS_MT",
    "QMAKE_CXXFLAGS_MT_DBG",
    "QMAKE_CXXFLAGS_MT_DLL",
    "QMAKE_CXXFLAGS_MT_DLLDBG",
    "QMAKE_CXXFLAGS_RELEASE",
    "QMAKE_CXXFLAGS_SHLIB",
    "QMAKE_CXXFLAGS_THREAD",
    "QMAKE_CXXFLAGS_WARN_OFF",
    "QMAKE_CXXFLAGS_WARN_ON",
    "QMAKE_DISTCLEAN",
    "QMAKE_EXTENSION_SHLIB",
    "QMAKE_EXTRA_COMPILERS",
    "QMAKE_EXTRA_TARGETS",
    "QMAKE_EXT_CPP",
    "QMAKE_EXT_H",
    "QMAKE_EXT_LEX",
    "QMAKE_EXT_MOC",
    "QMAKE_EXT_OBJ",
    "QMAKE_EXT_PRL",
    "QMAKE_EXT_UI",
    "QMAKE_EXT_YACC",
    "QMAKE_FAILED_REQUIREMENTS",
    "QMAKE_FRAMEWORK_BUNDLE_NAME",
    "QMAKE_FRAMEWORK_VERSION",
    "QMAKE_INCDIR",
    "QMAKE_INCDIR_EGL",
    "QMAKE_INCDIR_OPENGL",
    "QMAKE_INCDIR_OPENGL_ES1",
    "QMAKE_INCDIR_OPENGL_ES2",
    "QMAKE_INCDIR_OPENVG",
    "QMAKE_INCDIR_QT",
    "QMAKE_INCDIR_THREAD",
    "QMAKE_INCDIR_X11",
    "QMAKE_INFO_PLIST",
    "QMAKE_LFLAGS",
    "QMAKE_LFLAGS_CONSOLE",
    "QMAKE_LFLAGS_CONSOLE_DLL",
    "QMAKE_LFLAGS_DEBUG",
    "QMAKE_LFLAGS_PLUGIN",
    "QMAKE_LFLAGS_QT_DLL",
    "QMAKE_LFLAGS_RELEASE",
    "QMAKE_LFLAGS_RPATH",
    "QMAKE_LFLAGS_SHAPP",
    "QMAKE_LFLAGS_SHLIB",
    "QMAKE_LFLAGS_SONAME",
    "QMAKE_LFLAGS_THREAD",
    "QMAKE_LFLAGS_WINDOWS",
    "QMAKE_LFLAGS_WINDOWS_DLL",
    "QMAKE_LIBDIR",
    "QMAKE_LIBDIR_EGL",
    "QMAKE_LIBDIR_FLAGS",
    "QMAKE_LIBDIR_OPENGL",
    "QMAKE_LIBDIR_OPENVG",
    "QMAKE_LIBDIR_QT",
    "QMAKE_LIBDIR_X11",
    "QMAKE_LIBS",
    "QMAKE_LIBS_CONSOLE",
    "QMAKE_LIBS_EGL",
    "QMAKE_LIBS_OPENGL",
    "QMAKE_LIBS_OPENGL_ES1",
    "QMAKE_LIBS_OPENGL_ES2",
    "QMAKE_LIBS_OPENGL_QT",
    "QMAKE_LIBS_OPENVG",
    "QMAKE_LIBS_QT",
    "QMAKE_LIBS_QT_DLL",
    "QMAKE_LIBS_QT_OPENGL",
    "QMAKE_LIBS_QT_THREAD",
    "QMAKE_LIBS_RT",
    "QMAKE_LIBS_RTMT",
    "QMAKE_LIBS_THREAD",
    "QMAKE_LIBS_WINDOWS",
    "QMAKE_LIBS_X11",
    "QMAKE_LIBS_X11SM",
    "QMAKE_LIB_FLAG",
    "QMAKE_LINK",
    "QMAKE_LINK_SHLIB_CMD",
    "QMAKE_LN_SHLIB",
    "QMAKE_MACOSX_DEPLOYMENT_TARGET",
    "QMAKE_MAC_SDK",
    "QMAKE_MAKEFILE",
    "QMAKE_MOC_SRC",
    "QMAKE_POST_LINK",
    "QMAKE_PRE_LINK",
    "QMAKE_PROJECT_NAME",
    "QMAKE_QMAKE",
    "QMAKE_QT_DLL",
    "QMAKE_RESOURCE_FLAGS",
    "QMAKE_RPATH",
    "QMAKE_RPATHDIR",
    "QMAKE_RUN_CC",
    "QMAKE_RUN_CC_IMP",
    "QMAKE_RUN_CXX",
    "QMAKE_RUN_CXX_IMP",
    "QMAKE_TARGET",
    "QMAKE_UIC",
    "QT",
    "QTPLUGIN",
    "QT_MAJOR_VERSION",
    "QT_MINOR_VERSION",
    "QT_PATCH_VERSION",
    "QT_VERSION",
    "RCC_DIR",
    "RC_FILE",
    "REQUIRES",
    "RESOURCES",
    "RES_FILE",
    "RSS_RULES",
    "SIGNATURE_FILE",
    "SOURCES",
    "SRCMOC",
    "STATECHARTS",
    "SUBDIRS",
    "TARGET",
    "TEMPLATE",
    "TRANSLATIONS",
    "UICIMPLS",
    "UICOBJECTS",
    "UI_DIR",
    "UI_HEADERS_DIR",
    "UI_SOURCES_DIR",
    "VERSION",
    "VERSION_PE_HEADER",
    "VER_MAJ",
    "VER_MIN",
    "VER_PAT",
    "VPATH",
    "YACCIMPLS",
    "YACCOBJECTS",
    "YACCSOURCES",
    "_PRO_FILE_",
    "_PRO_FILE_PWD_",
    0
};

static const char *const functionKeywords[] = {
    "CONFIG",
    "absolute_path",
    "basename",
    "cache",
    "cat",
    "clean_path",
    "clear",
    "contains",
    "count",
    "debug",
    "defined",
    "dirname",
    "enumerate_vars",
    "equals",
    "error",
    "escape_expand",
    "eval",
    "exists",
    "export",
    "files",
    "find",
    "first",
    "for",
    "format_number",
    "fromfile",
    "getenv",
    "greaterThan",
    "if",
    "include",
    "infile",
    "isActiveConfig",
    "isEmpty",
    "isEqual",
    "join",
    "last",
    "lessThan",
    "list",
    "load",
    "log",
    "lower",
    "member",
    "message",
    "mkpath",
    "packagesExist",
    "parseJson",
    "prompt",
    "quote",
    "re_escape",
    "relative_path",
    "replace",
    "requires",
    "resolve_depends",
    "reverse",
    "section",
    "shadowed",
    "shell_path",
    "shell_quote",
    "size",
    "sort_depends",
    "split",
    "sprintf",
    "system",
    "system_path",
    "system_quote",
    "title",
    "touch",
    "unique",
    "unset",
    "upper",
    "val_escape",
    "warning",
    "write_file",
    0
};

// -------------------------------
// ProFileCompletionAssistProvider
// -------------------------------
void ProFileCompletionAssistProvider::init()
{
    for (uint i = 0; i < sizeof variableKeywords / sizeof variableKeywords[0] - 1; i++)
        m_variables.append(QLatin1String(variableKeywords[i]));
    for (uint i = 0; i < sizeof functionKeywords / sizeof functionKeywords[0] - 1; i++)
        m_functions.append(QLatin1String(functionKeywords[i]));
}

ProFileCompletionAssistProvider::~ProFileCompletionAssistProvider()
{
}

bool ProFileCompletionAssistProvider::supportsEditor(Core::Id editorId) const
{
    return editorId == QmakeProjectManager::Constants::PROFILE_EDITOR_ID;
}

IAssistProcessor *ProFileCompletionAssistProvider::createProcessor() const
{
    if (m_variables.isEmpty())
        const_cast<ProFileCompletionAssistProvider *>(this)->init();
    TextEditor::Keywords keywords = TextEditor::Keywords(m_variables, m_functions, QMap<QString, QStringList>());
    auto processor = new KeywordsCompletionAssistProcessor(keywords);
    processor->setSnippetGroup(TextEditor::Constants::TEXT_SNIPPET_GROUP_ID);
    return processor;
}

QStringList ProFileCompletionAssistProvider::variables() const
{
    if (m_variables.isEmpty())
        const_cast<ProFileCompletionAssistProvider *>(this)->init();
    return m_variables;
}

QStringList ProFileCompletionAssistProvider::functions() const
{
    if (m_functions.isEmpty())
        const_cast<ProFileCompletionAssistProvider *>(this)->init();
    return m_functions;
}
