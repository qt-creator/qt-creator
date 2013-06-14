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

#ifndef PYTHONEDITOR_CONSTANTS_H
#define PYTHONEDITOR_CONSTANTS_H

#include <QtGlobal>

namespace PythonEditor {
namespace Constants {

const char C_PYTHONEDITOR_ID[] = "PythonEditor.PythonEditor";
const char C_EDITOR_DISPLAY_NAME[] =
        QT_TRANSLATE_NOOP("OpenWith::Editors", "Python Editor");

/*******************************************************************************
 * File creation wizard
 ******************************************************************************/
const char C_PY_WIZARD_CATEGORY[] = "U.Python";
const char C_PY_EXTENSION[] = ".py";
const char C_PY_DISPLAY_CATEGORY[] = "Python";

    // source
const char C_PY_SOURCE_WIZARD_ID[] = "P.PySource";
const char C_PY_SOURCE_CONTENT[] =
        "#!/usr/bin/env python\n"
        "# -*- coding: utf-8 -*-\n"
        "\n";
const char EN_PY_SOURCE_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("PythonEditor::FileWizard", "Python source file");
const char EN_PY_SOURCE_DESCRIPTION[] =
        QT_TRANSLATE_NOOP("PythonEditor::FileWizard", "Creates an empty Python script with UTF-8 charset");

    // class
const char C_PY_CLASS_WIZARD_ID[] = "P.PyClass";
const char EN_PY_CLASS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("PythonEditor::ClassWizard", "Python class");
const char EN_PY_CLASS_DESCRIPTION[] =
        QT_TRANSLATE_NOOP("PythonEditor::ClassWizard", "Creates new Python class");

    // For future: boost binding
const char C_PY_CPPMODULE_WIZARD_ID[] = "F.PyCppModule";
const char EN_PY_CPPMODULE_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("PythonEditor::ClassWizard", "C++ module for Python");
const char EN_PY_CPPMODULE_DESCRIPTION[] =
        QT_TRANSLATE_NOOP("PythonEditor::ClassWizard", "Creates C++/Boost file with bindings for Python");

/*******************************************************************************
 * MIME type
 ******************************************************************************/
const char C_PY_MIMETYPE[] = "text/x-python";
const char RC_PY_MIME_XML[] = ":/pythoneditor/PythonEditor.mimetypes.xml";
const char C_PY_MIME_ICON[] = "text-x-python";

} // namespace Constants
} // namespace PythonEditor

#endif // PYTHONEDITOR_CONSTANTS_H
