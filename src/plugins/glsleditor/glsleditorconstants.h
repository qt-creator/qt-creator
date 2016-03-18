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

#pragma once

#include <QtGlobal>

namespace GlslEditor {
namespace Constants {

const char M_CONTEXT[] = "GLSL Editor.ContextMenu";
const char M_TOOLS_GLSL[] = "GLSLEditor.Tools.Menu";

const char M_REFACTORING_MENU_INSERTION_POINT[] = "GLSLEditor.RefactorGroup";

const char C_GLSLEDITOR_ID[] = "GLSLEditor.GLSLEditor";
const char C_GLSLEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "GLSL Editor");

const char GLSL_MIMETYPE[] = "application/x-glsl";
const char GLSL_MIMETYPE_VERT[] = "text/x-glsl-vert";
const char GLSL_MIMETYPE_FRAG[] = "text/x-glsl-frag";
const char GLSL_MIMETYPE_VERT_ES[] = "text/x-glsl-es-vert";
const char GLSL_MIMETYPE_FRAG_ES[] = "text/x-glsl-es-frag";

const char WIZARD_CATEGORY_GLSL[] = "U.GLSL";
const char WIZARD_TR_CATEGORY_GLSL[] = QT_TRANSLATE_NOOP("GLSLEditor", "GLSL");

} // namespace Constants
} // namespace GlslEditor
