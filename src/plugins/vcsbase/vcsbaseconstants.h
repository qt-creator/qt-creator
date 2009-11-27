/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef VCSBASE_CONSTANTS_H
#define VCSBASE_CONSTANTS_H

#include <QtCore/QtGlobal>

namespace VCSBase {
namespace Constants {

const char * const VCS_SETTINGS_CATEGORY = "V.Version Control";
const char * const VCS_SETTINGS_TR_CATEGORY = QT_TRANSLATE_NOOP("VCSBase", "Version Control");
const char * const VCS_COMMON_SETTINGS_ID = "A.Common";
const char * const VCS_COMMON_SETTINGS_NAME = QT_TRANSLATE_NOOP("VCSBase", "Common");

const char * const VCS_WIZARD_TR_CATEGORY = QT_TRANSLATE_NOOP("VCSBase", "Version Control");
const char * const VCS_WIZARD_CATEGORY = "Z.Version Control";

// Ids for sort order (wizards and preferences)
const char * const VCS_ID_GIT = "G.Git";
const char * const VCS_ID_MERCURIAL = "H.Mercurial";
const char * const VCS_ID_SUBVERSION = "J.Subversion";
const char * const VCS_ID_PERFORCE = "P.Perforce";
const char * const VCS_ID_CVS = "Z.CVS";

namespace Internal {
    enum { debug = 0 };
} // namespace Internal

} // namespace Constants
} // VCSBase

#endif // VCSBASE_CONSTANTS_H
