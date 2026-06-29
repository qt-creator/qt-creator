// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Bazaar::Constants {

inline constexpr char BAZAAR[] = "bazaar";
inline constexpr char BAZAARREPO[] = ".bzr";
inline constexpr char BAZAARDEFAULT[] = "bzr";
inline constexpr char BAZAAR_CONTEXT[] = "Bazaar Context";

// Changeset identifiers
inline constexpr char CHANGESET_ID[] = "^("
                            "revno: [.0-9]+" // detailed
                            "| +[.0-9]+"     // short
                            "|[.0-9]+: "     // line
                            ")";
inline constexpr char CHANGESET_ID_EXACT[] = "^([.0-9]+)$";
inline constexpr char ANNOTATE_CHANGESET_ID[] = "([.0-9]+)";

// Base editor parameters
inline constexpr char FILELOG_ID[] = "Bazaar File Log Editor";
inline constexpr char LOGAPP[] = "text/vnd.qtcreator.bazaar.log";

inline constexpr char ANNOTATELOG_ID[] = "Bazaar Annotation Editor";
inline constexpr char ANNOTATEAPP[] = "text/vnd.qtcreator.bazaar.annotation";

inline constexpr char DIFFLOG_ID[] = "Bazaar Diff Editor";
inline constexpr char DIFFAPP[] = "text/x-patch";

// File status hint
inline constexpr char FSTATUS_UNKNOWN[] = "Unknown";
inline constexpr char FSTATUS_CREATED[] = "Created";
inline constexpr char FSTATUS_DELETED[] = "Deleted";
inline constexpr char FSTATUS_MODIFIED[] = "Modified";
inline constexpr char FSTATUS_RENAMED[] = "Renamed";

} // Bazaar::Constants
