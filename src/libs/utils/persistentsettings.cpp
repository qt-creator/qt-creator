/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "persistentsettings.h"

#include <utils/fileutils.h>

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QVariant>
#include <QStack>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDateTime>

#include <utils/qtcassert.h>


/*!
    \class Utils::PersistentSettingsReader

    \brief Reads a QVariantMap of arbitrary, nested data structures from a XML file.

    Handles all string-serializable simple types and QVariantList and QVariantMap. Example:
    \code
<qtcreator>
    <data>
        <variable>ProjectExplorer.Project.ActiveTarget</variable>
        <value type="int">0</value>
    </data>
    <data>
        <variable>ProjectExplorer.Project.EditorSettings</variable>
        <valuemap type="QVariantMap">
            <value type="bool" key="EditorConfiguration.AutoIndent">true</value>
        </valuemap>
    </data>
    \endcode

    When parsing the structure, a parse stack of ParseValueStackEntry is used for each
    <data> element. ParseValueStackEntry is a variant/union of:
    \list
    \o simple value
    \o map
    \o list
    \endlist

    When entering a value element ( \c <value> / \c <valuelist> , \c <valuemap> ), entry is pushed
    accordingly. When leaving the element, the QVariant-value of the entry is taken off the stack
    and added to the stack entry below (added to list or inserted into map). The first element
    of the stack is the value of the <data> element.

    \sa Utils::PersistentSettingsWriter
*/

namespace Utils {

struct Context // Basic context containing element name string constants.
{
    Context();

    const QString qtCreatorElement;
    const QString dataElement;
    const QString variableElement;
    const QString typeAttribute;
    const QString valueElement;
    const QString valueListElement;
    const QString valueMapElement;
    const QString keyAttribute;
};

Context::Context() :
    qtCreatorElement(QLatin1String("qtcreator")),
    dataElement(QLatin1String("data")),
    variableElement(QLatin1String("variable")),
    typeAttribute(QLatin1String("type")),
    valueElement(QLatin1String("value")),
    valueListElement(QLatin1String("valuelist")),
    valueMapElement(QLatin1String("valuemap")),
    keyAttribute(QLatin1String("key"))
{
}

struct ParseValueStackEntry
{
    explicit ParseValueStackEntry(QVariant::Type t = QVariant::Invalid, const QString &k = QString()) : type(t), key(k) {}
    explicit ParseValueStackEntry(const QVariant &aSimpleValue, const QString &k);

    QVariant value() const;
    void addChild(const QString &key, const QVariant &v);

    QVariant::Type type;
    QString key;
    QVariant simpleValue;
    QVariantList listValue;
    QVariantMap mapValue;
};

ParseValueStackEntry::ParseValueStackEntry(const QVariant &aSimpleValue, const QString &k) :
    type(aSimpleValue.type()), key(k), simpleValue(aSimpleValue)
{
    QTC_ASSERT(simpleValue.isValid(), return);
}

QVariant ParseValueStackEntry::value() const
{
    switch (type) {
    case QVariant::Invalid:
        return QVariant();
    case QVariant::Map:
        return QVariant(mapValue);
    case QVariant::List:
        return QVariant(listValue);
    default:
        break;
    }
    return simpleValue;
}

void ParseValueStackEntry::addChild(const QString &key, const QVariant &v)
{
    switch (type) {
    case QVariant::Map:
        mapValue.insert(key, v);
        break;
    case QVariant::List:
        listValue.push_back(v);
        break;
    default:
        qWarning() << "ParseValueStackEntry::Internal error adding " << key << v << " to "
                 << QVariant::typeToName(type) << value();
        break;
    }
}

class ParseContext : public Context
{
public:
    QVariantMap parse(QFile &file);

private:
    enum Element { QtCreatorElement, DataElement, VariableElement,
                   SimpleValueElement, ListValueElement, MapValueElement, UnknownElement };

    Element element(const QStringRef &r) const;
    static inline bool isValueElement(Element e)
        { return e == SimpleValueElement || e == ListValueElement || e == MapValueElement; }
    QVariant readSimpleValue(QXmlStreamReader &r, const QXmlStreamAttributes &attributes) const;

    bool handleStartElement(QXmlStreamReader &r);
    bool handleEndElement(const QStringRef &name);

    QStack<ParseValueStackEntry> m_valueStack;
    QVariantMap m_result;
    QString m_currentVariableName;
};

QVariantMap ParseContext::parse(QFile &file)
{
    QXmlStreamReader r(&file);

    m_result.clear();
    m_currentVariableName.clear();

    while (!r.atEnd()) {
        switch (r.readNext()) {
        case QXmlStreamReader::StartElement:
            if (handleStartElement(r))
                return m_result;
            break;
        case QXmlStreamReader::EndElement:
            if (handleEndElement(r.name()))
                return m_result;
            break;
        case QXmlStreamReader::Invalid:
            qWarning("Error reading %s:%d: %s", qPrintable(file.fileName()),
                     int(r.lineNumber()), qPrintable(r.errorString()));
            return QVariantMap();
            break;
        default:
            break;
        } // switch token
    } // while (!r.atEnd())
    return m_result;
}

bool ParseContext::handleStartElement(QXmlStreamReader &r)
{
    const QStringRef name = r.name();
    const Element e = element(name);
    if (e == VariableElement) {
        m_currentVariableName = r.readElementText();
        return false;
    }
    if (!ParseContext::isValueElement(e))
        return false;

    const QXmlStreamAttributes attributes = r.attributes();
    const QString key = attributes.hasAttribute(keyAttribute) ?
                attributes.value(keyAttribute).toString() : QString();
    switch (e) {
    case SimpleValueElement:
        // This reads away the end element, so, handle end element right here.
        m_valueStack.push_back(ParseValueStackEntry(readSimpleValue(r, attributes), key));
        return handleEndElement(name);
    case ListValueElement:
        m_valueStack.push_back(ParseValueStackEntry(QVariant::List, key));
        break;
    case MapValueElement:
        m_valueStack.push_back(ParseValueStackEntry(QVariant::Map, key));
        break;
    default:
        break;
    }
    return false;
}

bool ParseContext::handleEndElement(const QStringRef &name)
{
    const Element e = element(name);
    if (ParseContext::isValueElement(e)) {
        QTC_ASSERT(!m_valueStack.isEmpty(), return true);
        const ParseValueStackEntry top = m_valueStack.pop();
        if (m_valueStack.isEmpty()) { // Last element? -> Done with that variable.
            QTC_ASSERT(!m_currentVariableName.isEmpty(), return true);
            m_result.insert(m_currentVariableName, top.value());
            m_currentVariableName.clear();
            return false;
        }
        m_valueStack.top().addChild(top.key, top.value());
    }
    return e == QtCreatorElement;
}

ParseContext::Element ParseContext::element(const QStringRef &r) const
{
    if (r == valueElement)
        return SimpleValueElement;
    if (r == valueListElement)
        return ListValueElement;
    if (r == valueMapElement)
        return MapValueElement;
    if (r == qtCreatorElement)
        return QtCreatorElement;
    if (r == dataElement)
        return DataElement;
    if (r == variableElement)
        return VariableElement;
    return UnknownElement;
}

QVariant ParseContext::readSimpleValue(QXmlStreamReader &r, const QXmlStreamAttributes &attributes) const
{
    // Simple value
    const QString type = attributes.value(typeAttribute).toString();
    const QString text = r.readElementText();
    if (type == QLatin1String("QChar")) { // Workaround: QTBUG-12345
        QTC_ASSERT(text.size() == 1, return QVariant());
        return QVariant(QChar(text.at(0)));
    }
    QVariant value;
    value.setValue(text);
    value.convert(QVariant::nameToType(type.toLatin1().data()));
    return value;
}

// =================================== PersistentSettingsReader

PersistentSettingsReader::PersistentSettingsReader()
{
}

QVariant PersistentSettingsReader::restoreValue(const QString &variable) const
{
    if (m_valueMap.contains(variable))
        return m_valueMap.value(variable);
    return QVariant();
}

QVariantMap PersistentSettingsReader::restoreValues() const
{
    return m_valueMap;
}

bool PersistentSettingsReader::load(const Utils::FileName &fileName)
{
    m_valueMap.clear();

    QFile file(fileName.toString());
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return false;
    ParseContext ctx;
    m_valueMap = ctx.parse(file);
    file.close();
    return true;
}

/*!
    \class Utils::PersistentSettingsWriter

    \brief Serializes a QVariantMap of arbitrary, nested data structures to a XML file.
    \sa Utils::PersistentSettingsReader
*/

static void writeVariantValue(QXmlStreamWriter &w, const Context &ctx,
                              const QVariant &variant, const QString &key = QString())
{
    switch (static_cast<int>(variant.type())) {
    case static_cast<int>(QVariant::StringList):
    case static_cast<int>(QVariant::List):
        w.writeStartElement(ctx.valueListElement);
        w.writeAttribute(ctx.typeAttribute, QLatin1String(QVariant::typeToName(QVariant::List)));
        if (!key.isEmpty())
            w.writeAttribute(ctx.keyAttribute, key);
        foreach (const QVariant &var, variant.toList())
            writeVariantValue(w, ctx, var);
        w.writeEndElement();
        break;
    case static_cast<int>(QVariant::Map): {
        w.writeStartElement(ctx.valueMapElement);
        w.writeAttribute(ctx.typeAttribute, QLatin1String(QVariant::typeToName(QVariant::Map)));
        if (!key.isEmpty())
            w.writeAttribute(ctx.keyAttribute, key);
        const QVariantMap varMap = variant.toMap();
        const QVariantMap::const_iterator cend = varMap.constEnd();
        for (QVariantMap::const_iterator i = varMap.constBegin(); i != cend; ++i)
            writeVariantValue(w, ctx, i.value(), i.key());
        w.writeEndElement();
    }
    break;
    case static_cast<int>(QMetaType::QObjectStar): // ignore QObjects!
    case static_cast<int>(QMetaType::VoidStar): // ignore void pointers!
        break;
    default:
        w.writeStartElement(ctx.valueElement);
        w.writeAttribute(ctx.typeAttribute, QLatin1String(variant.typeName()));
        if (!key.isEmpty())
            w.writeAttribute(ctx.keyAttribute, key);
        w.writeCharacters(variant.toString());
        w.writeEndElement();
        break;
    }
}

PersistentSettingsWriter::PersistentSettingsWriter(const FileName &fileName, const QString &docType) :
    m_fileName(fileName), m_docType(docType)
{ }

PersistentSettingsWriter::~PersistentSettingsWriter()
{
    write(m_savedData, 0);
}

bool PersistentSettingsWriter::save(const QVariantMap &data, QWidget *parent) const
{
    if (data == m_savedData)
        return true;

    return write(data, parent);
}

FileName PersistentSettingsWriter::fileName() const
{ return m_fileName; }

bool PersistentSettingsWriter::write(const QVariantMap &data, QWidget *parent) const
{
    QDir tmp;
    tmp.mkpath(m_fileName.toFileInfo().path());
    Utils::FileSaver saver(m_fileName.toString(), QIODevice::Text);
    if (!saver.hasError()) {
        const Context ctx;
        QXmlStreamWriter w(saver.file());
        w.setAutoFormatting(true);
        w.setAutoFormattingIndent(1); // Historical, used to be QDom.
        w.writeStartDocument();
        w.writeDTD(QLatin1String("<!DOCTYPE ") + m_docType + QLatin1Char('>'));
        w.writeComment(QString::fromLatin1(" Written by %1 %2, %3. ").
                       arg(QCoreApplication::applicationName(),
                           QCoreApplication::applicationVersion(),
                           QDateTime::currentDateTime().toString(Qt::ISODate)));
        w.writeStartElement(ctx.qtCreatorElement);
        const QVariantMap::const_iterator cend = data.constEnd();
        for (QVariantMap::const_iterator it =  data.constBegin(); it != cend; ++it) {
            w.writeStartElement(ctx.dataElement);
            w.writeTextElement(ctx.variableElement, it.key());
            writeVariantValue(w, ctx, it.value());
            w.writeEndElement();
        }
        w.writeEndDocument();

        saver.setResult(&w);
    }
    bool ok = saver.finalize(parent);
    if (ok)
        m_savedData = data;
    return ok;
}

} // namespace Utils
