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

#ifndef GENERICHIGHLIGHTERCONSTANTS_H
#define GENERICHIGHLIGHTERCONSTANTS_H

#include <QtCore/QtGlobal>

namespace Highlight {
namespace Constants {

const char * const GENERIC_EDITOR = "GenericEditorPlugin.GenericEditor";
const char * const GENERIC_EDITOR_DISPLAY_NAME =
    QT_TRANSLATE_NOOP("OpenWith::Editors", "Generic Editor");

const char * const C_SOURCE_MIMETYPE = "text/x-csrc";
const char * const C_HEADER_MIMETYPE = "text/x-chdr";
const char * const CPP_SOURCE_MIMETYPE = "text/x-c++src";
const char * const CPP_HEADER_MIMETYPE = "text/x-c++hdr";
const char * const CSS_MIMETYPE = "text/css";
const char * const FORTRAN_MIMETYPE = "text/x-fortran";
const char * const HTML_MIMETYPE = "text/html";
const char * const JAVA_MIMETYPE = "text/x-java";
const char * const JAVASCRIPT_MIMETYPE = "application/x-javascript";
const char * const OBJECTIVEC_MIMETYPE = "text/x-objcsrc";
const char * const PERL_MIMETYPE = "application/x-perl";
const char * const PHP_MIMETYPE = "application/x-php";
const char * const PYTHON_MIMETYPE = "text/x-python";
const char * const RUBY_MIMETYPE = "text/x-ruby";
const char * const SQL_MIMETYPE = "text/x-sql";
const char * const TCL_MIMETYPE = "application/x-tcl";

} // namespace Constants
} // namespace Highlight

#endif // GENERICHIGHLIGHTERCONSTANTS_H
