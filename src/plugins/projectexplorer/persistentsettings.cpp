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

#include "persistentsettings.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QVariant>
#include <QtXml/QDomDocument>
#include <QtXml/QDomCDATASection>


using namespace ProjectExplorer;

PersistentSettingsReader::PersistentSettingsReader()
{

}

QVariant PersistentSettingsReader::restoreValue(const QString & variable) const
{
    const QString &name = m_prefix + variable;
    if (m_valueMap.contains(name))
        return m_valueMap.value(name);
    return QVariant();
}

bool PersistentSettingsReader::load(const QString & fileName)
{
    m_valueMap.clear();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDomDocument doc;
    if (!doc.setContent(&file))
        return false;

    QDomElement root = doc.documentElement();
    if (root.nodeName() != QLatin1String("qtcreator"))
        return false;

    QDomElement child = root.firstChildElement();
    for (; !child.isNull(); child = child.nextSiblingElement()) {
        if (child.nodeName() == QLatin1String("data"))
            readValues(child);
    }

    file.close();
    return true;
}

void PersistentSettingsReader::setPrefix(const QString &prefix)
{
    m_prefix = prefix;
}

QString PersistentSettingsReader::prefix() const
{
    return m_prefix;
}

QVariant PersistentSettingsReader::readValue(const QDomElement &valElement) const
{
    QString name = valElement.nodeName();
    QString type = valElement.attribute(QLatin1String("type"));
    QVariant v;

    if (name == QLatin1String("value")) {
        v.setValue(valElement.text());
        v.convert(QVariant::nameToType(type.toLatin1().data()));
    } else if (name == QLatin1String("valuelist")) {
        QDomElement child = valElement.firstChildElement();
        QList<QVariant> valList;
        for (; !child.isNull(); child = child.nextSiblingElement()) {
            valList << readValue(child);
        }
        v.setValue(valList);
    } else if (name == QLatin1String("valuemap")) {
        QDomElement child = valElement.firstChildElement();
        QMap<QString, QVariant> valMap;
        for (; !child.isNull(); child = child.nextSiblingElement()) {
            QString key = child.attribute(QLatin1String("key"));
            valMap.insert(key, readValue(child));
        }
        v.setValue(valMap);
    }

    return v;
}

void PersistentSettingsReader::readValues(const QDomElement &data)
{
    QString variable;
    QVariant v;

    QDomElement child = data.firstChildElement();
    for (; !child.isNull(); child = child.nextSiblingElement()) {
        if (child.nodeName() == QLatin1String("variable")) {
            variable = child.text();
        } else {
            v = readValue(child);
        }
    }

    m_valueMap.insert(variable, v);
}

///
///  PersistentSettingsWriter
///

PersistentSettingsWriter::PersistentSettingsWriter()
{

}

void PersistentSettingsWriter::writeValue(QDomElement &ps, const QVariant &variant)
{
    if (variant.type() == QVariant::StringList || variant.type() == QVariant::List) {
        QDomElement values = ps.ownerDocument().createElement("valuelist");
        values.setAttribute("type", QVariant::typeToName(QVariant::List));
        QList<QVariant> varList = variant.toList();
        foreach (QVariant var, varList) {
            writeValue(values, var);
        }
        ps.appendChild(values);
    } else if (variant.type() == QVariant::Map) {
        QDomElement values = ps.ownerDocument().createElement("valuemap");
        values.setAttribute("type", QVariant::typeToName(QVariant::Map));

        QMap<QString, QVariant> varMap = variant.toMap();
        QMap<QString, QVariant>::const_iterator i = varMap.constBegin();
        while (i != varMap.constEnd()) {
            writeValue(values, i.value());
            values.lastChild().toElement().
                setAttribute(QLatin1String("key"), i.key());
            ++i;
        }

        ps.appendChild(values);
    } else {
        QDomElement value = ps.ownerDocument().createElement("value");
        ps.appendChild(value);
        QDomText valueText = ps.ownerDocument().createTextNode(variant.toString());
        value.appendChild(valueText);
        value.setAttribute("type", variant.typeName());
        ps.appendChild(value);
    }
}

void PersistentSettingsWriter::saveValue(const QString & variable, const QVariant &value)
{
    m_valueMap[m_prefix + variable] = value;
}

bool PersistentSettingsWriter::save(const QString & fileName, const QString & docType)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QDomDocument doc(docType);

    QDomElement root = doc.createElement("qtcreator");
    doc.appendChild(root);

    QMap<QString, QVariant>::const_iterator i = m_valueMap.constBegin();
    while (i != m_valueMap.constEnd()) {
        QDomElement ps = doc.createElement("data");
        root.appendChild(ps);

        QDomElement variable = doc.createElement("variable");
        ps.appendChild(variable);
        QDomText variableText = doc.createTextNode(i.key());
        variable.appendChild(variableText);

        writeValue(ps, i.value());
        ++i;
    }

    file.write(doc.toByteArray());
    file.close();
    return true;
}

void PersistentSettingsWriter::setPrefix(const QString &prefix)
{
    m_prefix = prefix;
}

QString PersistentSettingsWriter::prefix() const
{
    return m_prefix;
}
