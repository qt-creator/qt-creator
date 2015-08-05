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

#ifndef VCSBASE_CONSTANTS_H
#define VCSBASE_CONSTANTS_H

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

#endif // VCSBASE_CONSTANTS_H
