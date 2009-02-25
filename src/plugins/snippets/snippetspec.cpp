/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "snippetspec.h"
#include "persistentsettings.h"

using namespace Snippets::Internal;
using ProjectExplorer::PersistentSettingsReader;

bool SnippetSpec::load(const QString &fileName)
{
    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return false;

    m_contents = reader.restoreValue(QLatin1String("Contents")).toString();
    m_name = reader.restoreValue(QLatin1String("Name")).toString();
    m_description = reader.restoreValue(QLatin1String("Description")).toString();
    m_category = reader.restoreValue(QLatin1String("Category")).toString();
    m_completionShortcut = reader.restoreValue(QLatin1String("Shortcut")).toString();

    QMap<QString, QVariant> temp = reader.restoreValue(QLatin1String("Arguments")).toMap();
    QMap<QString, QVariant>::const_iterator it, end;
    end = temp.constEnd();
    for (it = temp.constBegin(); it != end; ++it) {
        m_argumentDescription.insert( it.key().toInt(), it.value().toString());
    }

    temp = reader.restoreValue(QLatin1String("ArgumentDefaults")).toMap();
    end = temp.constEnd();
    for (it = temp.constBegin(); it != end; ++it) {
        m_argumentDefault.insert(it.key().toInt(), it.value().toString());
    }

    return true;
}

QString SnippetSpec::contents() const
{
    return m_contents;
}

QString SnippetSpec::name() const
{
    return m_name;
}

QString SnippetSpec::description() const
{
    return m_description;
}

QString SnippetSpec::category() const
{
    return m_category;
}

QString SnippetSpec::completionShortcut() const
{
    return m_completionShortcut;
}

QString SnippetSpec::argumentDescription(int id) const
{
    return m_argumentDescription.value(id);
}

QString SnippetSpec::argumentDefault(int id) const
{
    return m_argumentDefault.value(id);
}
