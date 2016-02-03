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

namespace VcsBase {
namespace Constants {

const char VCS_SETTINGS_CATEGORY[] = "V.Version Control";
const char VCS_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("VcsBase", "Version Control");
const char SETTINGS_CATEGORY_VCS_ICON[] = ":/vcsbase/images/category_vcs.png";
const char VCS_COMMON_SETTINGS_ID[] = "A.VCS.General";
const char VCS_COMMON_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("VcsBase", "General");

// Ids for sort order (wizards and preferences)
const char VCS_ID_BAZAAR[] = "B.Bazaar";
const char VCS_ID_GIT[] = "G.Git";
const char VCS_ID_MERCURIAL[] = "H.Mercurial";
const char VCS_ID_SUBVERSION[] = "J.Subversion";
const char VCS_ID_PERFORCE[] = "P.Perforce";
const char VCS_ID_CVS[] = "Z.CVS";

const char VAR_VCS_NAME[] = "CurrentProject:VcsName";
const char VAR_VCS_TOPIC[] = "CurrentProject:VcsTopic";
const char VAR_VCS_TOPLEVELPATH[] = "CurrentProject:VcsTopLevelPath";

} // namespace Constants
} // namespace VcsBase
