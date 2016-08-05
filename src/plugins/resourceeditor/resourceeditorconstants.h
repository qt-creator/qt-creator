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

namespace ResourceEditor {
namespace Constants {

const char C_RESOURCEEDITOR[] = "Qt4.ResourceEditor";
const char RESOURCEEDITOR_ID[] = "Qt4.ResourceEditor";
const char C_RESOURCEEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Resource Editor");

const char REFRESH[] = "ResourceEditor.Refresh";

const char C_RESOURCE_MIMETYPE[] = "application/vnd.qt.xml.resource";

const char C_ADD_PREFIX[] = "ResourceEditor.AddPrefix";
const char C_REMOVE_PREFIX[] = "ResourceEditor.RemovePrefix";
const char C_RENAME_PREFIX[] = "ResourceEditor.RenamePrefix";
const char C_REMOVE_NON_EXISTING[] = "ResourceEditor.RemoveNonExisting";

const char C_REMOVE_FILE[] = "ResourceEditor.RemoveFile";
const char C_RENAME_FILE[] = "ResourceEditor.RenameFile";

const char C_OPEN_EDITOR[] = "ResourceEditor.OpenEditor";

const char C_COPY_PATH[] = "ResourceEditor.CopyPath";
const char C_COPY_URL[] = "ResourceEditor.CopyUrl";

} // namespace Constants
} // namespace ResourceEditor
