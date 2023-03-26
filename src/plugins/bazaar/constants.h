// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Bazaar::Constants {

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
const char CHANGESET_ID_EXACT[] = "^([.0-9]+)$";
const char ANNOTATE_CHANGESET_ID[] = "([.0-9]+)";

// Base editor parameters
const char FILELOG_ID[] = "Bazaar File Log Editor";
const char FILELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Bazaar File Log Editor");
const char LOGAPP[] = "text/vnd.qtcreator.bazaar.log";

const char ANNOTATELOG_ID[] = "Bazaar Annotation Editor";
const char ANNOTATELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Bazaar Annotation Editor");
const char ANNOTATEAPP[] = "text/vnd.qtcreator.bazaar.annotation";

const char DIFFLOG_ID[] = "Bazaar Diff Editor";
const char DIFFLOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Bazaar Diff Editor");
const char DIFFAPP[] = "text/x-patch";

// File status hint
const char FSTATUS_CREATED[] = "Created";
const char FSTATUS_DELETED[] = "Deleted";
const char FSTATUS_MODIFIED[] = "Modified";
const char FSTATUS_RENAMED[] = "Renamed";

} // Bazaar::Constants
