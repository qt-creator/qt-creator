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

#ifndef CPPTOOLSCONSTANTS_H
#define CPPTOOLSCONSTANTS_H

#include <QtCore/QtGlobal>

namespace CppTools {
namespace Constants {

const char * const M_TOOLS_CPP              = "CppTools.Tools.Menu";
const char * const SWITCH_HEADER_SOURCE     = "CppTools.SwitchHeaderSource";
const char * const TASK_INDEX               = "CppTools.Task.Index";
const char * const TASK_SEARCH              = "CppTools.Task.Search";
const char * const C_SOURCE_MIMETYPE = "text/x-csrc";
const char * const C_HEADER_MIMETYPE = "text/x-chdr";
const char * const CPP_SOURCE_MIMETYPE = "text/x-c++src";
const char * const OBJECTIVE_CPP_SOURCE_MIMETYPE = "text/x-objcsrc";
const char * const CPP_HEADER_MIMETYPE = "text/x-c++hdr";

// QSettings keys for use by the "New Class" wizards.
const char * const CPPTOOLS_SETTINGSGROUP = "CppTools";
const char * const LOWERCASE_CPPFILES_KEY = "LowerCaseFiles";
enum { lowerCaseFilesDefault = 1 };

const char * const CPP_SETTINGS_ID = "File Naming";
const char * const CPP_SETTINGS_NAME = QT_TRANSLATE_NOOP("CppTools", "File Naming");
const char * const CPP_SETTINGS_CATEGORY = "I.C++";
const char * const CPP_SETTINGS_TR_CATEGORY = QT_TRANSLATE_NOOP("CppTools", "C++");

} // namespace Constants
} // namespace CppTools

#endif // CPPTOOLSCONSTANTS_H
