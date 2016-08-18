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

#include "snippetssettings.h"
#include "reuse.h"

#include <QSettings>

namespace {

static const QLatin1String kGroupPostfix("SnippetsSettings");
static const QLatin1String kLastUsedSnippetGroup("LastUsedSnippetGroup");

} // Anonymous

using namespace TextEditor;
using namespace Internal;

void SnippetsSettings::toSettings(const QString &category, QSettings *s) const
{
    const QString &group = category + kGroupPostfix;
    s->beginGroup(group);
    s->setValue(kLastUsedSnippetGroup, m_lastUsedSnippetGroup);
    s->endGroup();
}

void SnippetsSettings::fromSettings(const QString &category, QSettings *s)
{
    const QString &group = category + kGroupPostfix;
    s->beginGroup(group);
    m_lastUsedSnippetGroup = s->value(kLastUsedSnippetGroup, QString()).toString();
    s->endGroup();
}

void SnippetsSettings::setLastUsedSnippetGroup(const QString &lastUsed)
{
    m_lastUsedSnippetGroup = lastUsed;
}

const QString &SnippetsSettings::lastUsedSnippetGroup() const
{
    return m_lastUsedSnippetGroup;
}

bool SnippetsSettings::equals(const SnippetsSettings &snippetsSettings) const
{
    return m_lastUsedSnippetGroup == snippetsSettings.m_lastUsedSnippetGroup;
}
