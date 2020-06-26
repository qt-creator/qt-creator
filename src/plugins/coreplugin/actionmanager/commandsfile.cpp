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

#include "commandsfile.h"
#include "command_p.h"
#include <coreplugin/dialogs/shortcutsettings.h>
#include <coreplugin/icore.h>

#include <app/app_version.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QKeySequence>
#include <QFile>
#include <QXmlStreamAttributes>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDebug>
#include <QDateTime>

using namespace Utils;

namespace Core {
namespace Internal {

struct Context // XML parsing context with strings.
{
    Context();

    const QString mappingElement;
    const QString shortCutElement;
    const QString idAttribute;
    const QString keyElement;
    const QString valueAttribute;
};

Context::Context() :
    mappingElement(QLatin1String("mapping")),
    shortCutElement(QLatin1String("shortcut")),
    idAttribute(QLatin1String("id")),
    keyElement(QLatin1String("key")),
    valueAttribute(QLatin1String("value"))
{
}

/*!
    \class Core::Internal::CommandsFile
    \internal
    \inmodule QtCreator
    \brief The CommandsFile class provides a collection of import and export commands.
    \inheaderfile commandsfile.h
*/

/*!
    \internal
*/
CommandsFile::CommandsFile(const QString &filename)
    : m_filename(filename)
{

}

/*!
    \internal
*/
QMap<QString, QList<QKeySequence>> CommandsFile::importCommands() const
{
    QMap<QString, QList<QKeySequence>> result;

    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return result;

    Context ctx;
    QXmlStreamReader r(&file);

    QString currentId;

    while (!r.atEnd()) {
        switch (r.readNext()) {
        case QXmlStreamReader::StartElement: {
            const QStringRef name = r.name();
            if (name == ctx.shortCutElement) {
                currentId = r.attributes().value(ctx.idAttribute).toString();
                if (!result.contains(currentId))
                    result.insert(currentId, {});
            } else if (name == ctx.keyElement) {
                QTC_ASSERT(!currentId.isEmpty(), continue);
                const QXmlStreamAttributes attributes = r.attributes();
                if (attributes.hasAttribute(ctx.valueAttribute)) {
                    const QString keyString = attributes.value(ctx.valueAttribute).toString();
                    QList<QKeySequence> keys = result.value(currentId);
                    result.insert(currentId, keys << QKeySequence(keyString));
                }
            } // if key element
        } // case QXmlStreamReader::StartElement
        default:
            break;
        } // switch
    } // while !atEnd
    file.close();
    return result;
}

/*!
    \internal
*/

bool CommandsFile::exportCommands(const QList<ShortcutItem *> &items)
{
    Utils::FileSaver saver(m_filename, QIODevice::Text);
    if (!saver.hasError()) {
        const Context ctx;
        QXmlStreamWriter w(saver.file());
        w.setAutoFormatting(true);
        w.setAutoFormattingIndent(1); // Historical, used to be QDom.
        w.writeStartDocument();
        w.writeDTD(QLatin1String("<!DOCTYPE KeyboardMappingScheme>"));
        w.writeComment(QString::fromLatin1(" Written by %1, %2. ").
                       arg(ICore::versionString(),
                           QDateTime::currentDateTime().toString(Qt::ISODate)));
        w.writeStartElement(ctx.mappingElement);
        foreach (const ShortcutItem *item, items) {
            const Id id = item->m_cmd->id();
            if (item->m_keys.isEmpty() || item->m_keys.first().isEmpty()) {
                w.writeEmptyElement(ctx.shortCutElement);
                w.writeAttribute(ctx.idAttribute, id.toString());
            } else {
                w.writeStartElement(ctx.shortCutElement);
                w.writeAttribute(ctx.idAttribute, id.toString());
                for (const QKeySequence &k : item->m_keys) {
                    w.writeEmptyElement(ctx.keyElement);
                    w.writeAttribute(ctx.valueAttribute, k.toString());
                }
                w.writeEndElement(); // Shortcut
            }
        }
        w.writeEndElement();
        w.writeEndDocument();

        saver.setResult(&w);
    }
    return saver.finalize();
}

} // namespace Internal
} // namespace Core
