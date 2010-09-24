/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "snippetsparser.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLatin1String>
#include <QtCore/QLatin1Char>
#include <QtCore/QVariant>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QDebug>

using namespace TextEditor;

SnippetsParser::SnippetsParser(const QString &fileName) : m_fileName(fileName)
{}

const QList<CompletionItem> &SnippetsParser::execute(ICompletionCollector *collector,
                                                     const QIcon &icon,
                                                     int order)
{
    if (!QFile::exists(m_fileName)) {
        m_snippets.clear();
    } else {
        const QDateTime &lastModified = QFileInfo(m_fileName).lastModified();
        if (m_lastTrackedFileChange.isNull() || m_lastTrackedFileChange != lastModified) {
            m_snippets.clear();
            QFile file(m_fileName);
            file.open(QIODevice::ReadOnly);
            QXmlStreamReader xml(&file);
            if (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("snippets")) {
                    while (xml.readNextStartElement()) {
                        if (xml.name() == QLatin1String("snippet")) {
                            TextEditor::CompletionItem item(collector);
                            QString title;
                            QString data;
                            QString description = xml.attributes().value("description").toString();

                            while (!xml.atEnd()) {
                                xml.readNext();
                                if (xml.isEndElement()) {
                                    int i = 0;
                                    while (i < data.size() && data.at(i).isLetterOrNumber())
                                        ++i;
                                    title = data.left(i);
                                    item.text = title;
                                    if (!description.isEmpty()) {
                                        item.text +=  QLatin1Char(' ');
                                        item.text += description;
                                    }
                                    item.data = QVariant::fromValue(data);

                                    QString infotip = data;
                                    while (infotip.size() && infotip.at(infotip.size()-1).isSpace())
                                        infotip.chop(1);
                                    infotip.replace(QLatin1Char('\n'), QLatin1String("<br>"));
                                    infotip.replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
                                    {
                                        QString s = QLatin1String("<nobr>");
                                        int count = 0;
                                        for (int i = 0; i < infotip.count(); ++i) {
                                            if (infotip.at(i) != QChar::ObjectReplacementCharacter) {
                                                s += infotip.at(i);
                                                continue;
                                            }
                                            if (++count % 2) {
                                                s += QLatin1String("<b>");
                                            } else {
                                                if (infotip.at(i-1) == QChar::ObjectReplacementCharacter)
                                                    s += QLatin1String("...");
                                                s += QLatin1String("</b>");
                                            }
                                        }
                                        infotip = s;
                                    }
                                    item.details = infotip;

                                    item.icon = icon;
                                    item.order = order;
                                    item.isSnippet = true;
                                    m_snippets.append(item);
                                    break;
                                }

                                if (xml.isCharacters())
                                    data += xml.text();
                                else if (xml.isStartElement()) {
                                    if (xml.name() != QLatin1String("tab"))
                                        xml.raiseError(QLatin1String("invalid snippets file"));
                                    else {
                                        data += QChar::ObjectReplacementCharacter;
                                        data += xml.readElementText();
                                        data += QChar::ObjectReplacementCharacter;
                                    }
                                }
                            }
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                } else {
                    xml.skipCurrentElement();
                }
            }
            if (xml.hasError())
                qWarning() << m_fileName << xml.errorString() << xml.lineNumber() << xml.columnNumber();
            file.close();

            m_lastTrackedFileChange = lastModified;
        }
    }

    return m_snippets;
}
