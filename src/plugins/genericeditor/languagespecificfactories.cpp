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
#include "genericeditor.h"
#include "genericeditorplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

using namespace Highlight;
using namespace Internal;

// C
CFactory::CFactory(QObject *parent) : GenericEditorFactory(parent)
{
    addMimeType(QLatin1String(Highlight::Constants::C_HEADER_MIMETYPE));
    addMimeType(QLatin1String(Highlight::Constants::C_SOURCE_MIMETYPE));
}

GenericEditor *CFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kCDefinitionId, parent); }

// C++
CppFactory::CppFactory(QObject *parent) : GenericEditorFactory(parent)
{
    addMimeType(QLatin1String(Highlight::Constants::CPP_HEADER_MIMETYPE));
    addMimeType(QLatin1String(Highlight::Constants::CPP_SOURCE_MIMETYPE));
}

GenericEditor *CppFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kCppDefinitionId, parent); }

// Css
CssFactory::CssFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::CSS_MIMETYPE)); }

GenericEditor *CssFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kCssDefinitionId, parent); }

// Fortran
FortranFactory::FortranFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::FORTRAN_MIMETYPE)); }

GenericEditor *FortranFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kFortranDefinitionId, parent); }

// Html
HtmlFactory::HtmlFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::HTML_MIMETYPE)); }

GenericEditor *HtmlFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kHtmlDefinitionId, parent); }

// Java
JavaFactory::JavaFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::JAVA_MIMETYPE)); }

GenericEditor *JavaFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kJavaDefinitionId, parent); }

// Javascript
JavascriptFactory::JavascriptFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::JAVASCRIPT_MIMETYPE)); }

GenericEditor *JavascriptFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kJavascriptDefinitionId, parent); }

// ObjectiveC
ObjectiveCFactory::ObjectiveCFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::OBJECTIVEC_MIMETYPE)); }

GenericEditor *ObjectiveCFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kObjectiveCDefinitionId, parent); }

// Perl
PerlFactory::PerlFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::PERL_MIMETYPE)); }

GenericEditor *PerlFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kPerlDefinitionId, parent); }

// Php
PhpFactory::PhpFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::PHP_MIMETYPE)); }

GenericEditor *PhpFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kPhpDefinitionId, parent); }

// Python
PythonFactory::PythonFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::PYTHON_MIMETYPE)); }

GenericEditor *PythonFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kPythonDefinitionId, parent); }

// Ruby
RubyFactory::RubyFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::RUBY_MIMETYPE)); }

GenericEditor *RubyFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kRubyDefinitionId, parent); }

// SQL
SqlFactory::SqlFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::SQL_MIMETYPE)); }

GenericEditor *SqlFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kSqlDefinitionId, parent); }

// Tcl
TclFactory::TclFactory(QObject *parent) : GenericEditorFactory(parent)
{ addMimeType(QLatin1String(Highlight::Constants::TCL_MIMETYPE)); }

GenericEditor *TclFactory::createGenericEditor(QWidget *parent)
{ return new GenericEditor(GenericEditorPlugin::kTclDefinitionId, parent); }
