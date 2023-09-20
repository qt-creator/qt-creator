// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Python {
namespace Constants {

const char C_PYTHONEDITOR_ID[] = "PythonEditor.PythonEditor";
const char C_PYTHONRUNCONFIGURATION_ID[] = "PythonEditor.RunConfiguration.";

const char C_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::Core", "Python Editor");

const char C_PYTHONOPTIONS_PAGE_ID[] = "PythonEditor.OptionsPage";
const char C_PYLSCONFIGURATION_PAGE_ID[] = "PythonEditor.PythonLanguageServerConfiguration";
const char C_PYTHON_SETTINGS_CATEGORY[] = "P.Python";

const char PYTHON_OPEN_REPL[] = "Python.OpenRepl";
const char PYTHON_OPEN_REPL_IMPORT[] = "Python.OpenReplImport";
const char PYTHON_OPEN_REPL_IMPORT_TOPLEVEL[] = "Python.OpenReplImportToplevel";

const char PYLS_SETTINGS_ID[] = "Python.PyLSSettingsID";

/*******************************************************************************
 * MIME type
 ******************************************************************************/
const char C_PY_MIMETYPE[] = "text/x-python";
const char C_PY_GUI_MIMETYPE[] = "text/x-python-gui";
const char C_PY3_MIMETYPE[] = "text/x-python3";
const char C_PY_MIME_ICON[] = "text-x-python";

} // namespace Constants
} // namespace Python
