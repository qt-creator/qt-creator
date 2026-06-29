// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Python::Constants {

inline constexpr char C_PYTHONEDITOR_ID[] = "PythonEditor.PythonEditor";
inline constexpr char C_PYTHONRUNCONFIGURATION_ID[] = "PythonEditor.RunConfiguration.";

inline constexpr char C_PYTHONOPTIONS_PAGE_ID[] = "PythonEditor.OptionsPage";
inline constexpr char C_PYLSCONFIGURATION_PAGE_ID[] = "PythonEditor.PythonLanguageServerConfiguration";
inline constexpr char C_PYTHON_SETTINGS_CATEGORY[] = "P.Python";

inline constexpr char PYTHON_TOOLKIT_ASPECT_ID[] = "Python.Interpreter";

inline constexpr char PYTHON_OPEN_REPL[] = "Python.OpenRepl";
inline constexpr char PYTHON_OPEN_REPL_IMPORT[] = "Python.OpenReplImport";
inline constexpr char PYTHON_OPEN_REPL_IMPORT_TOPLEVEL[] = "Python.OpenReplImportToplevel";

inline constexpr char PYLS_SETTINGS_ID[] = "Python.PyLSSettingsID";

/*******************************************************************************
 * MIME type
 ******************************************************************************/
inline constexpr char C_PY_MIMETYPE[] = "text/x-python";
inline constexpr char C_PY_GUI_MIMETYPE[] = "text/x-python-gui";
inline constexpr char C_PY3_MIMETYPE[] = "text/x-python3";
inline constexpr char C_PY_MIME_ICON[] = "text-x-python";
inline constexpr char C_PY_PROJECT_MIME_TYPE[] = "text/x-python-project";
inline constexpr char C_PY_PROJECT_MIME_TYPE_LEGACY[] = "text/x-pyqt-project";
inline constexpr char C_PY_PROJECT_MIME_TYPE_TOML[] = "text/x-python-pyproject-toml";

} // namespace Python::Constants
