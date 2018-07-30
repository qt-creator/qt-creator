/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

namespace Cppcheck {
namespace Constants {

const char TEXTMARK_CATEGORY_ID[] = "Cppcheck";

const char OPTIONS_PAGE_ID[] = "Analyzer.Cppcheck.Settings";

const char SETTINGS_ID[] = "Cppcheck";
const char SETTINGS_BINARY[] = "binary";
const char SETTINGS_WARNING[] = "warning";
const char SETTINGS_STYLE[] = "style";
const char SETTINGS_PERFORMANCE[] = "performance";
const char SETTINGS_PORTABILITY[] = "portability";
const char SETTINGS_INFORMATION[] = "information";
const char SETTINGS_UNUSED_FUNCTION[] = "unusedFunction";
const char SETTINGS_MISSING_INCLUDE[] = "missingInclude";
const char SETTINGS_INCONCLUSIVE[] = "inconclusive";
const char SETTINGS_FORCE_DEFINES[] = "forceDefines";
const char SETTINGS_CUSTOM_ARGUMENTS[] = "customArguments";
const char SETTINGS_IGNORE_PATTERNS[] = "ignorePatterns";
const char SETTINGS_SHOW_OUTPUT[] = "showOutput";
const char SETTINGS_ADD_INCLUDE_PATHS[] = "addIncludePaths";
const char SETTINGS_GUESS_ARGUMENTS[] = "guessArguments";

const char CHECK_PROGRESS_ID[] = "Cppcheck.Cppcheck.CheckingTask";

} // namespace Constants
} // namespace Cppcheck
