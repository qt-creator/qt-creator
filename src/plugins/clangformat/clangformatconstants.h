/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

namespace ClangFormat {
namespace Constants {
static const char SETTINGS_FILE_NAME[] = ".clang-format";
static const char SETTINGS_FILE_ALT_NAME[] = "_clang-format";
static const char SAMPLE_FILE_NAME[] = "test.cpp";
static const char SETTINGS_ID[] = "ClangFormat";
static const char FORMAT_CODE_INSTEAD_OF_INDENT_ID[] = "ClangFormat.FormatCodeInsteadOfIndent";
static const char FORMAT_WHILE_TYPING_ID[] = "ClangFormat.FormatWhileTyping";
static const char FORMAT_CODE_ON_SAVE_ID[] = "ClangFormat.FormatCodeOnSave";
static const char OVERRIDE_FILE_ID[] = "ClangFormat.OverrideFile";
static const char OPEN_CURRENT_CONFIG_ID[] = "ClangFormat.OpenCurrentConfig";
} // namespace Constants
} // namespace ClangFormat
