// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace VcsBase::Constants {

inline constexpr char VCS_SETTINGS_CATEGORY[] = "V.Version Control";
inline constexpr char VCS_COMMON_SETTINGS_ID[] = "A.VCS.General";

// Ids for sort order (wizards and preferences)
inline constexpr char VCS_ID_BAZAAR[] = "B.Bazaar";
inline constexpr char VCS_ID_GIT[] = "G.Git";
inline constexpr char VCS_ID_MERCURIAL[] = "H.Mercurial";
inline constexpr char VCS_ID_SUBVERSION[] = "J.Subversion";
inline constexpr char VCS_ID_PERFORCE[] = "P.Perforce";
inline constexpr char VCS_ID_CVS[] = "Z.CVS";

inline constexpr char VAR_VCS_NAME[] = "CurrentDocument:Project:VcsName";
inline constexpr char VAR_VCS_TOPIC[] = "CurrentDocument:Project:VcsTopic";
inline constexpr char VAR_VCS_TOPLEVELPATH[] = "CurrentDocument:Project:VcsTopLevelPath";

} // namespace VcsBase::Constants
