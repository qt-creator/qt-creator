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

#include "commonvcssettings.h"

#include <utils/hostosinfo.h>

#include <QSettings>
#include <QDebug>

static const char settingsGroupC[] = "VCS";
static const char nickNameMailMapKeyC[] = "NickNameMailMap";
static const char nickNameFieldListFileKeyC[] = "NickNameFieldListFile";
static const char submitMessageCheckScriptKeyC[] = "SubmitMessageCheckScript";
static const char lineWrapKeyC[] = "LineWrap";
static const char lineWrapWidthKeyC[] = "LineWrapWidth";
static const char sshPasswordPromptKeyC[] = "SshPasswordPrompt";

static const int lineWrapWidthDefault = 72;
static const bool lineWrapDefault = true;

// Return default for the ssh-askpass command (default to environment)
static inline QString sshPasswordPromptDefault()
{
    const QByteArray envSetting = qgetenv("SSH_ASKPASS");
    if (!envSetting.isEmpty())
        return QString::fromLocal8Bit(envSetting);
    if (Utils::HostOsInfo::isWindowsHost())
        return QLatin1String("win-ssh-askpass");
    return QLatin1String("ssh-askpass");
}

namespace VcsBase {
namespace Internal {

CommonVcsSettings::CommonVcsSettings() :
    sshPasswordPrompt(sshPasswordPromptDefault()),
    lineWrap(lineWrapDefault),
    lineWrapWidth(lineWrapWidthDefault)
{
}

void CommonVcsSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(nickNameMailMapKeyC), nickNameMailMap);
    s->setValue(QLatin1String(nickNameFieldListFileKeyC), nickNameFieldListFile);
    s->setValue(QLatin1String(submitMessageCheckScriptKeyC), submitMessageCheckScript);
    s->setValue(QLatin1String(lineWrapKeyC), lineWrap);
    s->setValue(QLatin1String(lineWrapWidthKeyC), lineWrapWidth);
    // Do not store the default setting to avoid clobbering the environment.
    if (sshPasswordPrompt != sshPasswordPromptDefault())
        s->setValue(QLatin1String(sshPasswordPromptKeyC), sshPasswordPrompt);
    else
        s->remove(QLatin1String(sshPasswordPromptKeyC));
    s->endGroup();
}

void CommonVcsSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(settingsGroupC));
    nickNameMailMap = s->value(QLatin1String(nickNameMailMapKeyC), QString()).toString();
    nickNameFieldListFile = s->value(QLatin1String(nickNameFieldListFileKeyC), QString()).toString();
    submitMessageCheckScript = s->value(QLatin1String(submitMessageCheckScriptKeyC), QString()).toString();
    lineWrap = s->value(QLatin1String(lineWrapKeyC), lineWrapDefault).toBool();
    lineWrapWidth = s->value(QLatin1String(lineWrapWidthKeyC), lineWrapWidthDefault).toInt();
    sshPasswordPrompt = s->value(QLatin1String(sshPasswordPromptKeyC), sshPasswordPromptDefault()).toString();
    s->endGroup();
}

bool CommonVcsSettings::equals(const CommonVcsSettings &rhs) const
{
    return lineWrap == rhs.lineWrap
           && lineWrapWidth == rhs.lineWrapWidth
           && nickNameMailMap == rhs.nickNameMailMap
           && nickNameFieldListFile == rhs.nickNameFieldListFile
           && submitMessageCheckScript == rhs.submitMessageCheckScript
           && sshPasswordPrompt == rhs.sshPasswordPrompt;
}

QDebug operator<<(QDebug d,const CommonVcsSettings& s)
{
    d.nospace() << " lineWrap=" << s.lineWrap
            << " lineWrapWidth=" <<  s.lineWrapWidth
            << " nickNameMailMap='" <<  s.nickNameMailMap
            << "' nickNameFieldListFile='" << s.nickNameFieldListFile
            << "'submitMessageCheckScript='" << s.submitMessageCheckScript
            << "'sshPasswordPrompt='" << s.sshPasswordPrompt
            << "'\n";
    return d;
}

} // namespace Internal
} // namespace VcsBase
