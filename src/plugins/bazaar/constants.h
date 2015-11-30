/**************************************************************************
**
** Copyright (C) 2015 Hugues Delorme
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
#ifndef BAZAARCONSTANTS_H
#define BAZAARCONSTANTS_H

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

#endif // BAZAARCONSTANTS_H
