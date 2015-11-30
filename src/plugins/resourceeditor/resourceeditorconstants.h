/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef RESOURCEEDITOR_CONSTANTS_H
#define RESOURCEEDITOR_CONSTANTS_H

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
const char C_REMOVE_NON_EXISTING[] = "RessourceEditor.RemoveNonExistign";

const char C_REMOVE_FILE[] = "ResourceEditor.RemoveFile";
const char C_RENAME_FILE[] = "ResourceEditor.RenameFile";

const char C_OPEN_EDITOR[] = "ResourceEditor.OpenEditor";

const char C_COPY_PATH[] = "ResourceEditor.CopyPath";
const char C_COPY_URL[] = "ResourceEditor.CopyUrl";


} // namespace Constants
} // namespace ResourceEditor

#endif // RESOURCEEDITOR_CONSTANTS_H
