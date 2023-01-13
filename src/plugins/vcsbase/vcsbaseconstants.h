// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace VcsBase {
namespace Constants {

const char VCS_SETTINGS_CATEGORY[] = "V.Version Control";
const char VCS_COMMON_SETTINGS_ID[] = "A.VCS.General";

// Ids for sort order (wizards and preferences)
const char VCS_ID_BAZAAR[] = "B.Bazaar";
const char VCS_ID_GIT[] = "G.Git";
const char VCS_ID_MERCURIAL[] = "H.Mercurial";
const char VCS_ID_SUBVERSION[] = "J.Subversion";
const char VCS_ID_PERFORCE[] = "P.Perforce";
const char VCS_ID_CVS[] = "Z.CVS";

const char VAR_VCS_NAME[] = "CurrentDocument:Project:VcsName";
const char VAR_VCS_TOPIC[] = "CurrentDocument:Project:VcsTopic";
const char VAR_VCS_TOPLEVELPATH[] = "CurrentDocument:Project:VcsTopLevelPath";

} // namespace Constants
} // namespace VcsBase
