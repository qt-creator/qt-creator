// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "persistentsettings.h"

#include "fileutils.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QRect>
#include <QRegularExpression>
#include <QStack>
#include <QTextStream>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#ifdef QT_GUI_LIB
#include <QMessageBox>
#endif

// Read and write rectangle in X11 resource syntax "12x12+4+3"
static QString rectangleToString(const QRect &r)
{
    QString result;
    QTextStream str(&result);
    str << r.width() << 'x' << r.height();
    if (r.x() >= 0)
        str << '+';
    str << r.x();
    if (r.y() >= 0)
        str << '+';
    str << r.y();
    return result;
}

static QRect stringToRectangle(const QString &v)
{
    static QRegularExpression pattern("^(\\d+)x(\\d+)([-+]\\d+)([-+]\\d+)$");
    Q_ASSERT(pattern.isValid());
    const QRegularExpressionMatch match = pattern.match(v);
    return match.hasMatch() ?
        QRect(QPoint(match.captured(3).toInt(), match.captured(4).toInt()),
              QSize(match.captured(1).toInt(), match.captured(2).toInt())) :
        QRect();
}

/*!
    \class Utils::PersistentSettingsReader
    \inmodule QtCreator

    \brief The PersistentSettingsReader class reads a QVariantMap of arbitrary,
    nested data structures from an XML file.

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
    \li simple value
    \li map
    \li list
    \endlist

    You can register string-serialize functions for custom types by registering them in the Qt Meta
    type system. Example:
    \code
      QMetaType::registerConverter(&MyCustomType::toString);
      QMetaType::registerConverter<QString, MyCustomType>(&myCustomTypeFromString);
    \endcode

    When entering a value element ( \c <value> / \c <valuelist> , \c <valuemap> ), entry is pushed
    accordingly. When leaving the element, the QVariant-value of the entry is taken off the stack
    and added to the stack entry below (added to list or inserted into map). The first element
    of the stack is the value of the <data> element.

    \sa Utils::PersistentSettingsWriter
*/

namespace Utils {

const QString qtCreatorElement("qtcreator");
const QString dataElement("data");
const QString variableElement("variable");
const QString typeAttribute("type");
const QString valueElement("value");
const QString valueListElement("valuelist");
const QString valueMapElement("valuemap");
const QString keyAttribute("key");

struct ParseValueStackEntry
{
    explicit ParseValueStackEntry(QVariant::Type t = QVariant::Invalid, const QString &k = {}) : type(t), key(k) {}
    explicit ParseValueStackEntry(const QVariant &aSimpleValue, const QString &k);

    QVariant value() const;
    void addChild(const QString &key, const QVariant &v);

    QVariant::Type type;
    QString key;
    QVariant simpleValue;
    QVariantList listValue;
    QVariantMap mapValue;
};

ParseValueStackEntry::ParseValueStackEntry(const QVariant &aSimpleValue, const QString &k)
    : type(aSimpleValue.type()), key(k), simpleValue(aSimpleValue)
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

class ParseContext
{
public:
    QVariantMap parse(const FilePath &file);

private:
    QVariant readSimpleValue(QXmlStreamReader &r, const QXmlStreamAttributes &attributes) const;

    bool handleStartElement(QXmlStreamReader &r);
    bool handleEndElement(const QStringView name);

    static QString formatWarning(const QXmlStreamReader &r, const QString &message);

    QStack<ParseValueStackEntry> m_valueStack;
    QVariantMap m_result;
    QString m_currentVariableName;
};

QVariantMap ParseContext::parse(const FilePath &file)
{
    QXmlStreamReader r(file.fileContents().value_or(QByteArray()));

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
            return {};
        default:
            break;
        } // switch token
    } // while (!r.atEnd())
    return m_result;
}

bool ParseContext::handleStartElement(QXmlStreamReader &r)
{
    const QStringView name = r.name();
    if (name == variableElement) {
        m_currentVariableName = r.readElementText();
        return false;
    }
    if (name == valueElement) {
        const QXmlStreamAttributes attributes = r.attributes();
        const QString key = attributes.hasAttribute(keyAttribute) ?
                    attributes.value(keyAttribute).toString() : QString();
        // This reads away the end element, so, handle end element right here.
        const QVariant v = readSimpleValue(r, attributes);
        if (!v.isValid()) {
            qWarning() << ParseContext::formatWarning(r, QString::fromLatin1("Failed to read element \"%1\".").arg(name.toString()));
            return false;
        }
        m_valueStack.push_back(ParseValueStackEntry(v, key));
        return handleEndElement(name);
    }
    if (name == valueListElement) {
        const QXmlStreamAttributes attributes = r.attributes();
        const QString key = attributes.hasAttribute(keyAttribute) ?
                    attributes.value(keyAttribute).toString() : QString();
        m_valueStack.push_back(ParseValueStackEntry(QVariant::List, key));
        return false;
    }
    if (name == valueMapElement) {
        const QXmlStreamAttributes attributes = r.attributes();
        const QString key = attributes.hasAttribute(keyAttribute) ?
                    attributes.value(keyAttribute).toString() : QString();
        m_valueStack.push_back(ParseValueStackEntry(QVariant::Map, key));
        return false;
    }
    return false;
}

bool ParseContext::handleEndElement(const QStringView name)
{
    if (name == valueElement || name == valueListElement || name == valueMapElement) {
        QTC_ASSERT(!m_valueStack.isEmpty(), return true);
        const ParseValueStackEntry top = m_valueStack.pop();
        if (m_valueStack.isEmpty()) { // Last element? -> Done with that variable.
            QTC_ASSERT(!m_currentVariableName.isEmpty(), return true);
            m_result.insert(m_currentVariableName, top.value());
            m_currentVariableName.clear();
            return false;
        }
        m_valueStack.top().addChild(top.key, top.value());
        return false;
    }
    return name == qtCreatorElement;
}

QString ParseContext::formatWarning(const QXmlStreamReader &r, const QString &message)
{
    QString result = QLatin1String("Warning reading ");
    if (const QIODevice *device = r.device())
        if (const auto file = qobject_cast<const QFile *>(device))
            result += QDir::toNativeSeparators(file->fileName()) + QLatin1Char(':');
    result += QString::number(r.lineNumber());
    result += QLatin1String(": ");
    result += message;
    return result;
}

QVariant ParseContext::readSimpleValue(QXmlStreamReader &r, const QXmlStreamAttributes &attributes) const
{
    // Simple value
    const QStringView type = attributes.value(typeAttribute);
    const QString text = r.readElementText();
    if (type == QLatin1String("QChar")) { // Workaround: QTBUG-12345
        QTC_ASSERT(text.size() == 1, return QVariant());
        return QVariant(QChar(text.at(0)));
    }
    if (type == QLatin1String("QRect")) {
        const QRect rectangle = stringToRectangle(text);
        return rectangle.isValid() ? QVariant(rectangle) : QVariant();
    }
    QVariant value;
    value.setValue(text);
    value.convert(QMetaType::type(type.toLatin1().constData()));
    return value;
}

// =================================== PersistentSettingsReader

PersistentSettingsReader::PersistentSettingsReader() = default;

QVariant PersistentSettingsReader::restoreValue(const Key &variable, const QVariant &defaultValue) const
{
    if (m_valueMap.contains(stringFromKey(variable)))
        return m_valueMap.value(stringFromKey(variable));
    return defaultValue;
}

Store PersistentSettingsReader::restoreValues() const
{
    return storeFromMap(m_valueMap);
}

bool PersistentSettingsReader::load(const FilePath &fileName)
{
    m_valueMap.clear();

    if (fileName.fileSize() == 0) // skip empty files
        return false;

    m_filePath = fileName.parentDir();
    ParseContext ctx;
    m_valueMap = ctx.parse(fileName);
    return true;
}

FilePath PersistentSettingsReader::filePath()
{
    return m_filePath;
}

/*!
    \class Utils::PersistentSettingsWriter
    \inmodule QtCreator

    \brief The PersistentSettingsWriter class serializes a Store of
    arbitrary, nested data structures to an XML file.
    \sa Utils::PersistentSettingsReader
*/

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
static QString xmlAttrFromKey(const QString &key) { return key; }
#else
static QString xmlAttrFromKey(const QString &key) { return key; }
#endif

static void writeVariantValue(QXmlStreamWriter &w, const QVariant &variant, const QString &key = {})
{
    static const int storeId = qMetaTypeId<Store>();

    const int variantType = variant.typeId();
    if (variantType == QMetaType::QStringList || variantType == QMetaType::QVariantList) {
        w.writeStartElement(valueListElement);
        w.writeAttribute(typeAttribute, "QVariantList");
        if (!key.isEmpty())
            w.writeAttribute(keyAttribute, xmlAttrFromKey(key));
        const QList<QVariant> list = variant.toList();
        for (const QVariant &var : list)
            writeVariantValue(w, var);
        w.writeEndElement();
    } else if (variantType == storeId || variantType == QMetaType::QVariantMap) {
        w.writeStartElement(valueMapElement);
        w.writeAttribute(typeAttribute, "QVariantMap");
        if (!key.isEmpty())
            w.writeAttribute(keyAttribute, xmlAttrFromKey(key));
        const QVariantMap varMap = variant.toMap();
        const auto cend = varMap.constEnd();
        for (auto i = varMap.constBegin(); i != cend; ++i)
            writeVariantValue(w, i.value(), i.key());
        w.writeEndElement();
    } else if (variantType == QMetaType::QObjectStar) {
        // ignore QObjects
    } else if (variantType == QMetaType::VoidStar) {
        // ignore void pointers
    } else {
        w.writeStartElement(valueElement);
        w.writeAttribute(typeAttribute, QLatin1String(variant.typeName()));
        if (!key.isEmpty())
            w.writeAttribute(keyAttribute, xmlAttrFromKey(key));
        switch (variant.type()) {
        case QVariant::Rect:
            w.writeCharacters(rectangleToString(variant.toRect()));
            break;
        default:
            w.writeCharacters(variant.toString());
            break;
        }
        w.writeEndElement();
    }
}

PersistentSettingsWriter::PersistentSettingsWriter(const FilePath &fileName, const QString &docType) :
    m_fileName(fileName), m_docType(docType)
{ }

bool PersistentSettingsWriter::save(const Store &data, QString *errorString) const
{
    if (data == m_savedData)
        return true;
    return write(data, errorString);
}

#ifdef QT_GUI_LIB
bool PersistentSettingsWriter::save(const Store &data, QWidget *parent) const
{
    QString errorString;
    const bool success = save(data, &errorString);
    if (!success)
        QMessageBox::critical(parent, Tr::tr("File Error"), errorString);
    return success;
}
#endif // QT_GUI_LIB

FilePath PersistentSettingsWriter::fileName() const
{ return m_fileName; }

//** * @brief Set contents of file (e.g. from data read from it). */
void PersistentSettingsWriter::setContents(const Store &data)
{
    m_savedData = data;
}

bool PersistentSettingsWriter::write(const Store &data, QString *errorString) const
{
    m_fileName.parentDir().ensureWritableDir();
    FileSaver saver(m_fileName, QIODevice::Text);
    if (!saver.hasError()) {
        QXmlStreamWriter w(saver.file());
        w.setAutoFormatting(true);
        w.setAutoFormattingIndent(1); // Historical, used to be QDom.
        w.writeStartDocument();
        w.writeDTD(QLatin1String("<!DOCTYPE ") + m_docType + QLatin1Char('>'));
        w.writeComment(QString::fromLatin1(" Written by %1 %2, %3. ").
                       arg(QCoreApplication::applicationName(),
                           QCoreApplication::applicationVersion(),
                           QDateTime::currentDateTime().toString(Qt::ISODate)));
        w.writeStartElement(qtCreatorElement);
        const QVariantMap map = mapFromStore(data);
        for (auto it = map.constBegin(), cend = map.constEnd(); it != cend; ++it) {
            w.writeStartElement(dataElement);
            w.writeTextElement(variableElement, it.key());
            writeVariantValue(w, it.value());
            w.writeEndElement();
        }
        w.writeEndDocument();

        saver.setResult(&w);
    }
    bool ok = saver.finalize();
    if (ok) {
        m_savedData = data;
    } else if (errorString) {
        m_savedData.clear();
        *errorString = saver.errorString();
    }

    return ok;
}

} // namespace Utils
