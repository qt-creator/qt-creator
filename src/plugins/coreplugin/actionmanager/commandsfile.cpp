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

#include "commandsfile.h"
#include "shortcutsettings.h"
#include "command_p.h"

#include <coreplugin/uniqueidmanager.h>

#include <QtCore/QFile>
#include <QtXml/QDomDocument>

using namespace Core;
using namespace Core::Internal;

/*!
    \class CommandsFile
    \brief The CommandsFile class provides a collection of import and export commands.
    \ingroup qwb
    \inheaderfile commandsfile.h
*/

/*!
    ...
*/
CommandsFile::CommandsFile(const QString &filename)
    : m_filename(filename)
{

}

/*!
    ...
*/
QMap<QString, QKeySequence> CommandsFile::importCommands() const
{
    QMap<QString, QKeySequence> result;

    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly))
        return result;

    QDomDocument doc("KeyboardMappingScheme");
    if (!doc.setContent(&file))
        return result;

    QDomElement root = doc.documentElement();
    if (root.nodeName() != QLatin1String("mapping"))
        return result;

    QDomElement ks = root.firstChildElement();
    for (; !ks.isNull(); ks = ks.nextSiblingElement()) {
        if (ks.nodeName() == QLatin1String("shortcut")) {
            QString id = ks.attribute(QLatin1String("id"));
            QKeySequence shortcutkey;
            QDomElement keyelem = ks.firstChildElement("key");
            if (!keyelem.isNull())
                shortcutkey = QKeySequence(keyelem.attribute("value"));
            result.insert(id, shortcutkey);
        }
    }

    file.close();
    return result;
}

/*!
    ...
*/
bool CommandsFile::exportCommands(const QList<ShortcutItem *> &items)
{
    UniqueIDManager *idmanager = UniqueIDManager::instance();

    QFile file(m_filename);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QDomDocument doc("KeyboardMappingScheme");
    QDomElement root = doc.createElement("mapping");
    doc.appendChild(root);

    foreach (const ShortcutItem *item, items) {
        QDomElement ctag = doc.createElement("shortcut");
        ctag.setAttribute(QLatin1String("id"), idmanager->stringForUniqueIdentifier(item->m_cmd->id()));
        root.appendChild(ctag);

        QDomElement ktag = doc.createElement("key");
        ktag.setAttribute(QLatin1String("value"), item->m_key.toString());
        ctag.appendChild(ktag);
    }

    file.write(doc.toByteArray());
    file.close();
    return true;
}
