/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
