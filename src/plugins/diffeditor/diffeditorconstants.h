/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtGlobal>

namespace DiffEditor {
namespace Constants {

const char DIFF_EDITOR_PLUGIN[] = "DiffEditorPlugin";

const char DIFF_EDITOR_ID[] = "Diff Editor";
const char DIFF_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("DiffEditor", "Diff Editor");
const char DIFF_EDITOR_MIMETYPE[] = "text/x-patch";
const char C_DIFF_EDITOR_DESCRIPTION[] = "DiffEditor.Description";
const char SIDE_BY_SIDE_VIEW_ID[] = "DiffEditor.SideBySide";
const char UNIFIED_VIEW_ID[] = "DiffEditor.Unified";

const char G_TOOLS_DIFF[] = "QtCreator.Group.Tools.Options";

const char EXPAND_BRANCHES[] = "Branches: <Expand>";

} // namespace Constants
} // namespace DiffEditor
