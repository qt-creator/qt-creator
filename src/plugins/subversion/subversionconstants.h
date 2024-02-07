// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Subversion::Constants {

const char SUBVERSION_PLUGIN[] = "SubversionPlugin";

const char NON_INTERACTIVE_OPTION[] = "--non-interactive";
enum { debug = 0 };

const char SUBVERSION_CONTEXT[]        = "Subversion Context";

const char SUBVERSION_COMMIT_EDITOR_ID[]  = "Subversion Commit Editor";
const char SUBVERSION_SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.svn.submit";

const char SUBVERSION_LOG_EDITOR_ID[] = "Subversion File Log Editor";
const char SUBVERSION_LOG_MIMETYPE[] = "text/vnd.qtcreator.svn.log";

const char SUBVERSION_BLAME_EDITOR_ID[] = "Subversion Annotation Editor";
const char SUBVERSION_BLAME_MIMETYPE[] = "text/vnd.qtcreator.svn.annotation";

} // Subversion::Constants
