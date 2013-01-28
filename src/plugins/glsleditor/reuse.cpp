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

#include "reuse.h"

#include <QLatin1String>

#include <glsl/glsllexer.h>

using namespace GLSL;

namespace GLSLEditor {
namespace Internal {

int languageVariant(const QString &mimeType)
{
    int variant = 0;
    bool isVertex = false;
    bool isFragment = false;
    bool isDesktop = false;
    if (mimeType.isEmpty()) {
        // ### Before file has been opened, so don't know the mime type.
        isVertex = true;
        isFragment = true;
    } else if (mimeType == QLatin1String("text/x-glsl") ||
               mimeType == QLatin1String("application/x-glsl")) {
        isVertex = true;
        isFragment = true;
        isDesktop = true;
    } else if (mimeType == QLatin1String("text/x-glsl-vert")) {
        isVertex = true;
        isDesktop = true;
    } else if (mimeType == QLatin1String("text/x-glsl-frag")) {
        isFragment = true;
        isDesktop = true;
    } else if (mimeType == QLatin1String("text/x-glsl-es-vert")) {
        isVertex = true;
    } else if (mimeType == QLatin1String("text/x-glsl-es-frag")) {
        isFragment = true;
    }
    if (isDesktop)
        variant |= Lexer::Variant_GLSL_120;
    else
        variant |= Lexer::Variant_GLSL_ES_100;
    if (isVertex)
        variant |= Lexer::Variant_VertexShader;
    if (isFragment)
        variant |= Lexer::Variant_FragmentShader;
    return variant;
}

} // Internal
} // GLSLEditor

