// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scxmlnamespace.h"

using namespace ScxmlEditor::PluginInterface;

ScxmlNamespace::ScxmlNamespace(const QString &prefix, const QString &name, QObject *parent)
    : QObject(parent)
    , m_prefix(prefix)
    , m_name(name)
{
}

QString ScxmlNamespace::prefix() const
{
    return m_prefix;
}

QString ScxmlNamespace::name() const
{
    return m_name;
}

bool ScxmlNamespace::isTagVisible(const QString &tag) const
{
    return m_tagVisibility.value(tag, true);
}

void ScxmlNamespace::setTagVisibility(const QString &tag, bool visible)
{
    m_tagVisibility[tag] = visible;
}
