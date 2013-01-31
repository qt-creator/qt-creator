/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
static const char patchCommandKeyC[] = "PatchCommand";
static const char patchCommandDefaultC[] = "patch";

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
    patchCommand(QLatin1String(patchCommandDefaultC)),
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
    s->setValue(QLatin1String(patchCommandKeyC), patchCommand);
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
    patchCommand = s->value(QLatin1String(patchCommandKeyC), QLatin1String(patchCommandDefaultC)).toString();
    s->endGroup();
}

bool CommonVcsSettings::equals(const CommonVcsSettings &rhs) const
{
    return lineWrap == rhs.lineWrap
           && lineWrapWidth == rhs.lineWrapWidth
           && nickNameMailMap == rhs.nickNameMailMap
           && nickNameFieldListFile == rhs.nickNameFieldListFile
           && submitMessageCheckScript == rhs.submitMessageCheckScript
           && sshPasswordPrompt == rhs.sshPasswordPrompt
           && patchCommand == rhs.patchCommand;
}

QDebug operator<<(QDebug d,const CommonVcsSettings& s)
{
    d.nospace() << " lineWrap=" << s.lineWrap
            << " lineWrapWidth=" <<  s.lineWrapWidth
            << " nickNameMailMap='" <<  s.nickNameMailMap
            << "' nickNameFieldListFile='" << s.nickNameFieldListFile
            << "'submitMessageCheckScript='" << s.submitMessageCheckScript
            << "'sshPasswordPrompt='" << s.sshPasswordPrompt
            << "'patchCommand='" << s.patchCommand
            << "'\n";
    return d;
}

} // namespace Internal
} // namespace VcsBase
