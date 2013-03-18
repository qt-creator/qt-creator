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

#include "pythoneditorplugin.h"
#include "pythoneditorconstants.h"
#include "wizard/pythonfilewizard.h"
#include "wizard/pythonclasswizard.h"
#include "pythoneditorwidget.h"
#include "pythoneditorfactory.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QtPlugin>
#include <QCoreApplication>

using namespace PythonEditor::Constants;

/*******************************************************************************
 * List of Python keywords (includes "print" that isn't keyword in python 3
 ******************************************************************************/
static const char *const LIST_OF_PYTHON_KEYWORDS[] = {
    "and",
    "as",
    "assert",
    "break",
    "class",
    "continue",
    "def",
    "del",
    "elif",
    "else",
    "except",
    "exec",
    "finally",
    "for",
    "from",
    "global",
    "if",
    "import",
    "in",
    "is",
    "lambda",
    "not",
    "or",
    "pass",
    "print",
    "raise",
    "return",
    "try",
    "while",
    "with",
    "yield"
};

/*******************************************************************************
 * List of Python magic methods and attributes
 ******************************************************************************/
static const char *const LIST_OF_PYTHON_MAGICS[] = {
    // ctor & dtor
    "__init__",
    "__del__",
    // string conversion functions
    "__str__",
    "__repr__",
    "__unicode__",
    // attribute access functions
    "__setattr__",
    "__getattr__",
    "__delattr__",
    // binary operators
    "__add__",
    "__sub__",
    "__mul__",
    "__truediv__",
    "__floordiv__",
    "__mod__",
    "__pow__",
    "__and__",
    "__or__",
    "__xor__",
    "__eq__",
    "__ne__",
    "__gt__",
    "__lt__",
    "__ge__",
    "__le__",
    "__lshift__",
    "__rshift__",
    "__contains__",
    // unary operators
    "__pos__",
    "__neg__",
    "__inv__",
    "__abs__",
    "__len__",
    // item operators like []
    "__getitem__",
    "__setitem__",
    "__delitem__",
    "__getslice__",
    "__setslice__",
    "__delslice__",
    // other functions
    "__cmp__",
    "__hash__",
    "__nonzero__",
    "__call__",
    "__iter__",
    "__reversed__",
    "__divmod__",
    "__int__",
    "__long__",
    "__float__",
    "__complex__",
    "__hex__",
    "__oct__",
    "__index__",
    "__copy__",
    "__deepcopy__",
    "__sizeof__",
    "__trunc__",
    "__format__",
    // magic attributes
    "__name__",
    "__module__",
    "__dict__",
    "__bases__",
    "__doc__"
};

/*******************************************************************************
 * List of python built-in functions and objects
 ******************************************************************************/
static const char *const LIST_OF_PYTHON_BUILTINS[] = {
    "range",
    "xrange",
    "int",
    "float",
    "long",
    "hex",
    "oct"
    "chr",
    "ord",
    "len",
    "abs",
    "None",
    "True",
    "False"
};

namespace PythonEditor {

PythonEditorPlugin *PythonEditorPlugin::m_instance = 0;

/// Copies identifiers from array to QSet
static void copyIdentifiers(const char * const words[], size_t bytesCount, QSet<QString> &result)
{
    const size_t count = bytesCount / sizeof(const char * const);
    for (size_t i = 0; i < count; ++i)
        result.insert(QLatin1String(words[i]));
}

PythonEditorPlugin::PythonEditorPlugin()
    : m_factory(0)
    , m_actionHandler(0)
{
    m_instance = this;
    copyIdentifiers(LIST_OF_PYTHON_KEYWORDS, sizeof(LIST_OF_PYTHON_KEYWORDS), m_keywords);
    copyIdentifiers(LIST_OF_PYTHON_MAGICS, sizeof(LIST_OF_PYTHON_MAGICS), m_magics);
    copyIdentifiers(LIST_OF_PYTHON_BUILTINS, sizeof(LIST_OF_PYTHON_BUILTINS), m_builtins);
}

PythonEditorPlugin::~PythonEditorPlugin()
{
    removeObject(m_factory);
    m_instance = 0;
}

bool PythonEditorPlugin::initialize(
        const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    if (! Core::ICore::mimeDatabase()->addMimeTypes(
            QLatin1String(RC_PY_MIME_XML),
            errorMessage))
    {
        return false;
    }

    m_factory = new EditorFactory(this);
    addObject(m_factory);

    ////////////////////////////////////////////////////////////////////////////
    // Initialize editor actions handler
    ////////////////////////////////////////////////////////////////////////////
    m_actionHandler.reset(new TextEditor::TextEditorActionHandler(
                              C_PYTHONEDITOR_ID,
                              TextEditor::TextEditorActionHandler::Format
                              | TextEditor::TextEditorActionHandler::UnCommentSelection
                              | TextEditor::TextEditorActionHandler::UnCollapseAll));
    m_actionHandler->initializeActions();

    ////////////////////////////////////////////////////////////////////////////
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    ////////////////////////////////////////////////////////////////////////////
    const QIcon icon = QIcon::fromTheme(QLatin1String(C_PY_MIME_ICON));
    if (!icon.isNull()) {
        Core::FileIconProvider *iconProv = Core::FileIconProvider::instance();
        Core::MimeDatabase *mimeDB = Core::ICore::instance()->mimeDatabase();
        iconProv->registerIconOverlayForMimeType(
                    icon, mimeDB->findByType(QLatin1String(C_PY_MIMETYPE)));
    }

    ////////////////////////////////////////////////////////////////////////////
    // Add Python files and classes creation dialogs
    ////////////////////////////////////////////////////////////////////////////
    addAutoReleasedObject(new FileWizard(Core::ICore::instance()));
    addAutoReleasedObject(new ClassWizard(Core::ICore::instance()));

    return true;
}

void PythonEditorPlugin::extensionsInitialized()
{
}

void PythonEditorPlugin::initializeEditor(EditorWidget *widget)
{
    instance()->m_actionHandler->setupActions(widget);
    TextEditor::TextEditorSettings::instance()->initializeEditor(widget);
}

QSet<QString> PythonEditorPlugin::keywords()
{
    return instance()->m_keywords;
}

QSet<QString> PythonEditorPlugin::magics()
{
    return instance()->m_magics;
}

QSet<QString> PythonEditorPlugin::builtins()
{
    return instance()->m_builtins;
}

} // namespace PythonEditor

Q_EXPORT_PLUGIN(PythonEditor::PythonEditorPlugin)

