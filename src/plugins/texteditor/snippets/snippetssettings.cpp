// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
