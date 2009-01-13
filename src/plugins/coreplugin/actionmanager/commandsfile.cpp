/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "coreimpl.h"
#include "commandsfile.h"
#include "shortcutsettings.h"
#include "command.h"

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
    UniqueIDManager *idmanager = CoreImpl::instance()->uniqueIDManager();

    QFile file(m_filename);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QDomDocument doc("KeyboardMappingScheme");
    QDomElement root = doc.createElement("mapping");
    doc.appendChild(root);

    for (int i=0; i<items.count(); ++i) {
        ShortcutItem *item = items.at(i);
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
