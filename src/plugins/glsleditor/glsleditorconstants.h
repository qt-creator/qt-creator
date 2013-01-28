/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef GLSLEDITOR_CONSTANTS_H
#define GLSLEDITOR_CONSTANTS_H

#include <QtGlobal>

namespace GLSLEditor {
namespace Constants {

// menus
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
} // namespace GLSLEditor

#endif // GLSLEDITOR_CONSTANTS_H
