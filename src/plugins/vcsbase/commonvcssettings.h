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

#include <QString>

QT_BEGIN_NAMESPACE
class QSettings;
class QDebug;
QT_END_NAMESPACE

namespace VcsBase {
namespace Internal {

// Common VCS settings, message check script and user nick names.
class CommonVcsSettings
{
public:
    CommonVcsSettings();

    QString nickNameMailMap;
    QString nickNameFieldListFile;

    QString submitMessageCheckScript;

    // Executable run to graphically prompt for a SSH-password.
    QString sshPasswordPrompt;

    bool lineWrap;
    int lineWrapWidth;

    void toSettings(QSettings *) const;
    void fromSettings(QSettings *);

    bool equals(const CommonVcsSettings &rhs) const;
};

inline bool operator==(const CommonVcsSettings &s1, const CommonVcsSettings &s2) { return s1.equals(s2); }
inline bool operator!=(const CommonVcsSettings &s1, const CommonVcsSettings &s2) { return !s1.equals(s2); }

QDebug operator<<(QDebug, const CommonVcsSettings &);

} // namespace Internal
} // namespace VcsBase
