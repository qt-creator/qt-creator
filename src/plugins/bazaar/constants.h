/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

namespace Bazaar {
namespace Constants {

const char BAZAAR[] = "bazaar";
const char BAZAARREPO[] = ".bzr";
const char BAZAARDEFAULT[] = "bzr";
const char BAZAAR_CONTEXT[] = "Bazaar Context";

// Changeset identifiers
const char CHANGESET_ID[] = "^("
                            "revno: [.0-9]+" // detailed
                            "| +[.0-9]+"     // short
                            "|[.0-9]+: "     // line
                            ")";
const char CHANGESET_ID_EXACT[] = "([.0-9]+)";
const char ANNOTATE_CHANGESET_ID[] = "([.0-9]+)";

// Base editor parameters
const char FILELOG_ID[] = "Bazaar File Log Editor";
const char FILELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Bazaar File Log Editor");
const char LOGAPP[] = "text/vnd.qtcreator.bazaar.log";

const char ANNOTATELOG_ID[] = "Bazaar Annotation Editor";
const char ANNOTATELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Bazaar Annotation Editor");
const char ANNOTATEAPP[] = "text/vnd.qtcreator.bazaar.annotation";

const char DIFFLOG_ID[] = "Bazaar Diff Editor";
const char DIFFLOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Bazaar Diff Editor");
const char DIFFAPP[] = "text/x-patch";

// File status hint
const char FSTATUS_CREATED[] = "Created";
const char FSTATUS_DELETED[] = "Deleted";
const char FSTATUS_MODIFIED[] = "Modified";
const char FSTATUS_RENAMED[] = "Renamed";

} // namespace Constants
} // namespace Bazaar
