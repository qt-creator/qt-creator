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

#include "languagespecificfactories.h"
#include "genericeditorconstants.h"
#include "editor.h"
#include "genericeditorplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

using namespace GenericEditor;
using namespace Internal;

// C
CFactory::CFactory(QObject *parent) : EditorFactory(parent)
{
    addMimeType(QLatin1String(GenericEditor::Constants::C_HEADER_MIMETYPE));
    addMimeType(QLatin1String(GenericEditor::Constants::C_SOURCE_MIMETYPE));
}

Editor *CFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kCDefinitionId, parent); }

// C++
CppFactory::CppFactory(QObject *parent) : EditorFactory(parent)
{
    addMimeType(QLatin1String(GenericEditor::Constants::CPP_HEADER_MIMETYPE));
    addMimeType(QLatin1String(GenericEditor::Constants::CPP_SOURCE_MIMETYPE));
}

Editor *CppFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kCppDefinitionId, parent); }

// Css
CssFactory::CssFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::CSS_MIMETYPE)); }

Editor *CssFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kCssDefinitionId, parent); }

// Fortran
FortranFactory::FortranFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::FORTRAN_MIMETYPE)); }

Editor *FortranFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kFortranDefinitionId, parent); }

// Html
HtmlFactory::HtmlFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::HTML_MIMETYPE)); }

Editor *HtmlFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kHtmlDefinitionId, parent); }

// Java
JavaFactory::JavaFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::JAVA_MIMETYPE)); }

Editor *JavaFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kJavaDefinitionId, parent); }

// Javascript
JavascriptFactory::JavascriptFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::JAVASCRIPT_MIMETYPE)); }

Editor *JavascriptFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kJavascriptDefinitionId, parent); }

// ObjectiveC
ObjectiveCFactory::ObjectiveCFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::OBJECTIVEC_MIMETYPE)); }

Editor *ObjectiveCFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kObjectiveCDefinitionId, parent); }

// Perl
PerlFactory::PerlFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::PERL_MIMETYPE)); }

Editor *PerlFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kPerlDefinitionId, parent); }

// Php
PhpFactory::PhpFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::PHP_MIMETYPE)); }

Editor *PhpFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kPhpDefinitionId, parent); }

// Python
PythonFactory::PythonFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::PYTHON_MIMETYPE)); }

Editor *PythonFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kPythonDefinitionId, parent); }

// Ruby
RubyFactory::RubyFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::RUBY_MIMETYPE)); }

Editor *RubyFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kRubyDefinitionId, parent); }

// SQL
SqlFactory::SqlFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::SQL_MIMETYPE)); }

Editor *SqlFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kSqlDefinitionId, parent); }

// Tcl
TclFactory::TclFactory(QObject *parent) : EditorFactory(parent)
{ addMimeType(QLatin1String(GenericEditor::Constants::TCL_MIMETYPE)); }

Editor *TclFactory::createGenericEditor(QWidget *parent)
{ return new Editor(GenericEditorPlugin::kTclDefinitionId, parent); }
