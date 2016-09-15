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

#include "scxmltag.h"
#include "scxmldocument.h"
#include "scxmleditorconstants.h"
#include "scxmltagutils.h"

#include <QCoreApplication>
#include <QDebug>

using namespace ScxmlEditor::PluginInterface;

ScxmlTag::ScxmlTag(TagType type, ScxmlDocument *document)
    : m_prefix(
          QLatin1String(type == Metadata || type == MetadataItem ? "qt" : ""))
{
    setDocument(document);
    init(type);
    m_tagName = QLatin1String(m_info->name);
}

ScxmlTag::ScxmlTag(const QString &prefix, const QString &name,
    ScxmlDocument *document)
    : m_tagName(name)
    , m_prefix(prefix)
{
    setDocument(document);
    TagType type = UnknownTag;
    for (int i = 0; i < Finalize; ++i) {
        if (QLatin1String(scxml_tags[i].name) == name) {
            type = (TagType)i;
            break;
        }
    }

    init(type);
}

ScxmlTag::ScxmlTag(const ScxmlTag *other, bool copyChildren)
{
    setDocument(other->m_document);
    m_tagType = other->m_tagType;
    m_tagName = other->m_tagName;
    m_content = other->m_content;
    m_prefix = other->m_prefix;
    m_info = &scxml_tags[m_tagType];
    m_attributeNames = other->m_attributeNames;
    m_attributeValues = other->m_attributeValues;
    m_editorInfo = other->m_editorInfo;

    if (copyChildren) {
        for (int i = 0; i < other->m_childTags.count(); ++i)
            appendChild(new ScxmlTag(other->m_childTags[i], copyChildren));
    }
}

ScxmlTag::~ScxmlTag()
{
    if (m_document)
        m_document->removeChild(this);

    m_attributeNames.clear();
    m_attributeValues.clear();
    m_childTags.clear();

    // m_parentTag = 0;
    m_document = nullptr;
    m_info = nullptr;
    m_tagType = UnknownTag;
}

void ScxmlTag::finalizeTagNames()
{
    switch (m_tagType) {
    case State: {
        if (hasAttribute("initial")) {
            QString oldInitial = attribute("initial");
            QString newInitial = m_document->m_idMap.value(oldInitial);
            setAttribute("initial", newInitial);
        }
        break;
    }
    default:
        break;
    }

    for (int i = m_childTags.count(); i--;) {
        ScxmlTag *t = m_childTags[i];
        switch (t->tagType()) {
        case InitialTransition:
        case Transition: {
            QString oldTarget = t->attribute("target");
            QString newTarget = m_document->m_idMap.value(oldTarget);
            if (!oldTarget.isEmpty() && newTarget.isEmpty())
                delete m_childTags.takeAt(i);
            else
                t->setAttribute("target", newTarget);
            break;
        }
        default:
            t->finalizeTagNames();
            break;
        }
    }
}

void ScxmlTag::print()
{
    qDebug() << "type            " << m_tagType;
    qDebug() << "name            " << m_tagName;
    qDebug() << "parent          " << (m_parentTag ? m_parentTag->m_tagName : "");
    qDebug() << "attributeNames  " << m_attributeNames;
    qDebug() << "attributeValues " << m_attributeValues;
    qDebug() << "childcount " << m_childTags.count();
    for (int i = 0; i < m_childTags.count(); ++i)
        qDebug() << " child           " << i << m_childTags[i]->m_tagName;
}

void ScxmlTag::init(TagType type)
{
    m_tagType = type;
    m_info = &scxml_tags[type];

    for (int i = 0; i < m_info->n_attributes; ++i) {
        if (m_info->attributes[i].value)
            setAttribute(
                QLatin1String(m_info->attributes[i].name),
                QString::fromLatin1(m_info->attributes[i].value).split(";").first());
    }

    initId();
}

void ScxmlTag::initId()
{
    // Init state IDs
    if (m_document) {
        switch (m_tagType) {
        case State:
            setAttribute("id", m_document->nextUniqueId("State"));
            break;
        case Parallel:
            setAttribute("id", m_document->nextUniqueId("Parallel"));
            break;
        case History:
            setAttribute("id", m_document->nextUniqueId("History"));
            break;
        case Transition:
            setAttribute("event", m_document->nextUniqueId("Transition"));
            break;
        case Final:
            setAttribute("id", m_document->nextUniqueId("Final"));
            break;
        default:
            break;
        }
    }
}

void ScxmlTag::setDocument(ScxmlDocument *document)
{
    if (m_document != document) {
        if (m_document)
            m_document->removeChild(this);

        m_document = document;
        if (m_document)
            m_document->addChild(this);
    }
}

ScxmlDocument *ScxmlTag::document() const
{
    return m_document;
}

const scxmltag_type_t *ScxmlTag::info() const
{
    return m_info;
}

TagType ScxmlTag::tagType() const
{
    return m_tagType;
}

QString ScxmlTag::prefix() const
{
    return m_prefix;
}

void ScxmlTag::setTagName(const QString &name)
{
    m_tagName = name;
}

QString ScxmlTag::tagName(bool addPrefix) const
{
    if (m_prefix.isEmpty() || !addPrefix)
        return m_tagName;
    else
        return QString::fromLatin1("%1:%2").arg(m_prefix).arg(m_tagName);
}

QString ScxmlTag::displayName() const
{
    switch (m_tagType) {
    case State:
    case Parallel:
    case Final:
        return attribute("id");
    case Transition:
    case InitialTransition:
        return attribute("event");
        break;
    default:
        return QString();
    }
}

QString ScxmlTag::stateNameSpace() const
{
    if (m_parentTag) {
        switch (m_parentTag->m_tagType) {
        case State:
        case Parallel:
            return QString::fromLatin1("%1%2%3")
                .arg(m_parentTag->stateNameSpace())
                .arg(m_parentTag->attribute(
                    m_parentTag->m_attributeNames.indexOf("id")))
                .arg(m_document->nameSpaceDelimiter());
        default:
            break;
        }
    }

    return QString();
}

QString ScxmlTag::content() const
{
    return m_content;
}

void ScxmlTag::setContent(const QString &content)
{
    m_content = content.trimmed();
}

bool ScxmlTag::hasData() const
{
    if (m_attributeNames.count() > 0 || !m_content.isEmpty())
        return true;

    foreach (ScxmlTag *tag, m_childTags) {
        if (tag->hasData())
            return true;
    }

    return false;
}

bool ScxmlTag::hasChild(TagType type) const
{
    foreach (ScxmlTag *tag, m_childTags) {
        if (tag->tagType() == type)
            return true;
    }

    return false;
}

bool ScxmlTag::hasChild(const QString &name) const
{
    foreach (ScxmlTag *tag, m_childTags) {
        if (tag->tagName() == name)
            return true;
    }

    return false;
}

void ScxmlTag::insertChild(int index, ScxmlTag *child)
{
    if (index >= 0 && index < m_childTags.count()) {
        m_childTags.insert(index, child);
        child->setParentTag(this);
    } else
        appendChild(child);
}

void ScxmlTag::appendChild(ScxmlTag *child)
{
    if (!m_childTags.contains(child)) {
        m_childTags.append(child);
        child->setParentTag(this);
    }
}

void ScxmlTag::removeChild(ScxmlTag *child)
{
    m_childTags.removeAll(child);
}

void ScxmlTag::moveChild(int oldPos, int newPos)
{
    m_childTags.insert(newPos, m_childTags.takeAt(oldPos));
}

QString ScxmlTag::attributeName(int ind) const
{
    if (ind >= 0 && ind < m_attributeNames.count())
        return m_attributeNames[ind];

    return QString();
}

QString ScxmlTag::attribute(int ind, const QString &defaultValue) const
{
    if (ind >= 0 && ind < m_attributeValues.count())
        return m_attributeValues[ind];

    return defaultValue;
}

void ScxmlTag::setEditorInfo(const QString &key, const QString &value)
{
    if (value.isEmpty())
        m_editorInfo.remove(key);
    else
        m_editorInfo[key] = value;
}

QString ScxmlTag::editorInfo(const QString &key) const
{
    return m_editorInfo.value(key);
}

bool ScxmlTag::hasEditorInfo(const QString &key) const
{
    return m_editorInfo.keys().contains(key);
}

void ScxmlTag::setAttributeName(int ind, const QString &name)
{
    if (m_attributeNames.contains(name))
        return;

    if (ind >= 0 && ind < m_attributeValues.count()) {
        m_attributeNames[ind] = name;
    } else {
        m_attributeNames << name;
        m_attributeValues << QCoreApplication::translate(
            "SXCMLTag::UnknownAttributeValue", "Unknown");
    }
}

void ScxmlTag::setAttribute(int ind, const QString &value)
{
    if (ind >= 0 && ind < m_attributeNames.count())
        setAttribute(m_attributeNames[ind], value);
    else {
        m_attributeNames << QCoreApplication::translate(
            "SXCMLTag::UnknownAttributeName", "Unknown");
        m_attributeValues << value;
    }
}

void ScxmlTag::setAttribute(const QString &name, const QString &value)
{
    if (value.isEmpty()) {
        int ind = m_attributeNames.indexOf(name);
        if (ind >= 0 && ind < m_attributeNames.count()) {
            m_attributeNames.removeAt(ind);
            m_attributeValues.removeAt(ind);
        }
    } else if (name.isEmpty()) {
        int ind = m_attributeValues.indexOf(value);
        if (ind >= 0 && ind < m_attributeValues.count()) {
            m_attributeNames.removeAt(ind);
            m_attributeValues.removeAt(ind);
        }
    } else {
        int ind = m_attributeNames.indexOf(name);
        if (ind >= 0 && ind < m_attributeNames.count()) {
            m_attributeNames[ind] = name;
            m_attributeValues[ind] = value;
        } else {
            m_attributeNames << name;
            m_attributeValues << value;
        }
    }
}

QStringList ScxmlTag::attributeNames() const
{
    return m_attributeNames;
}

QStringList ScxmlTag::attributeValues() const
{
    return m_attributeValues;
}

int ScxmlTag::attributeCount() const
{
    return m_attributeNames.count();
}

QString ScxmlTag::attribute(const QString &attr, bool useNameSpace,
    const QString &defaultValue) const
{
    QString value = attribute(m_attributeNames.indexOf(attr), defaultValue);
    if (useNameSpace && m_document->useFullNameSpace())
        return QString::fromLatin1("%1%2").arg(stateNameSpace()).arg(value);

    return value;
}

bool ScxmlTag::hasAttribute(const QString &key) const
{
    return m_attributeNames.contains(key);
}

int ScxmlTag::childCount() const
{
    return m_childTags.count();
}

int ScxmlTag::childIndex(const ScxmlTag *child) const
{
    return m_childTags.indexOf(const_cast<ScxmlTag*>(child));
}

void ScxmlTag::setParentTag(ScxmlTag *parentTag)
{
    m_parentTag = parentTag;
}

bool ScxmlTag::hasParentTag() const
{
    return m_parentTag != nullptr;
}

ScxmlTag *ScxmlTag::child(int index) const
{
    return index >= 0 && index < m_childTags.count()
        ? m_childTags.value(index)
        : 0;
}

ScxmlTag *ScxmlTag::parentTag() const
{
    return m_parentTag;
}

int ScxmlTag::index() const
{
    return m_parentTag ? m_parentTag->childIndex(const_cast<ScxmlTag*>(this))
                       : 0;
}

bool ScxmlTag::isRootTag() const
{
    return m_document->rootTag() == this;
}

QVector<ScxmlTag*> ScxmlTag::allChildren() const
{
    return m_childTags;
}

QVector<ScxmlTag*> ScxmlTag::children(const QString &name) const
{
    QVector<ScxmlTag*> children;
    foreach (ScxmlTag *tag, m_childTags) {
        if (tag->tagName() == name)
            children << tag;
    }

    return children;
}

ScxmlTag *ScxmlTag::child(const QString &name) const
{
    foreach (ScxmlTag *tag, m_childTags) {
        if (tag->tagName() == name)
            return tag;
    }

    return nullptr;
}

void ScxmlTag::writeXml(QXmlStreamWriter &xml)
{
    // Start tag
    xml.writeStartElement(tagName());

    // Don't write attribute if type is Script and content is available
    if (m_tagType != Script || m_content.isEmpty()) {
        for (int i = 0; i < m_attributeNames.count(); ++i) {
            if (m_attributeNames[i] == "id" && m_document->useFullNameSpace())
                xml.writeAttribute("id", attribute("id", true));
            else
                xml.writeAttribute(m_attributeNames[i], m_attributeValues[i]);
        }
    }

    // If tag is Scxml (root), we need to fine-tune initial-state
    if (m_tagType == Scxml) {
        setEditorInfo("initialGeometry", QString());
        setEditorInfo("transitionGeometry", QString());

        ScxmlTag *initialTag = child("initial");
        if (initialTag) {
            setEditorInfo("initialGeometry", initialTag->editorInfo(Constants::C_SCXML_EDITORINFO_GEOMETRY));

            ScxmlTag *initialTransitionTag = initialTag->child("transition");
            if (initialTransitionTag) {
                xml.writeAttribute("initial",
                    initialTransitionTag->attribute("target"));
                setEditorInfo("transitionGeometry",
                    initialTransitionTag->editorInfo(Constants::C_SCXML_EDITORINFO_GEOMETRY));
            }
        }
    }

    // Write content if necessary
    if (m_info->canIncludeContent && !m_content.isEmpty())
        xml.writeCharacters(m_content);

    // Write editorinfo if necessary
    if (!m_editorInfo.isEmpty()) {
        xml.writeStartElement("qt:editorinfo");
        QHashIterator<QString, QString> i(m_editorInfo);
        while (i.hasNext()) {
            i.next();
            xml.writeAttribute(i.key(), i.value());
        }
        xml.writeEndElement();
    }

    // Write children states
    for (int i = 0; i < m_childTags.count(); ++i) {
        if (m_tagType == Scxml && m_childTags[i]->tagType() == Initial)
            continue;

        if ((m_childTags[i]->tagType() == Metadata || m_childTags[i]->tagType() == MetadataItem) && !m_childTags[i]->hasData())
            continue;

        m_childTags[i]->writeXml(xml);
    }

    // End tag
    xml.writeEndElement();
}

void ScxmlTag::readXml(QXmlStreamReader &xml, bool checkCopyId)
{
    QString scxmlInitial;

    // Read and set attributes
    QXmlStreamAttributes attributes = xml.attributes();
    for (int i = 0; i < attributes.count(); ++i) {
        if (m_tagType == Scxml && attributes[i].qualifiedName() == "initial")
            scxmlInitial = attributes[i].value().toString();
        else {
            QString key = attributes[i].qualifiedName().toString();
            QString value = attributes[i].value().toString();

            // Modify id-attribute if necessary
            switch (m_tagType) {
            case History:
            case Final:
            case State:
            case Parallel:
            case Initial: {
                if (key == "id") {
                    setAttribute(key, value);

                    QString oldId = value;
                    QString parentNS = stateNameSpace();
                    if (value.startsWith(parentNS))
                        value.remove(parentNS);

                    if (checkCopyId)
                        value = m_document->getUniqueCopyId(this);
                    if (m_document->useFullNameSpace())
                        m_document->m_idMap[oldId] = QString::fromLatin1("%1%2").arg(stateNameSpace()).arg(value);
                    else
                        m_document->m_idMap[oldId] = value;
                }
                break;
            }
            default:
                break;
            }

            setAttribute(key, value);
        }
    }

    // Read children states
    QXmlStreamReader::TokenType token = xml.readNext();
    while (token != QXmlStreamReader::EndElement) {
        if (token == QXmlStreamReader::Characters)
            m_content = xml.text().toString().trimmed();
        else if (token == QXmlStreamReader::StartElement) {
            if (m_tagType != Metadata && m_tagType != MetadataItem && xml.qualifiedName().toString() == "qt:editorinfo") {
                // Read editorinfos
                QXmlStreamAttributes attributes = xml.attributes();
                foreach (QXmlStreamAttribute attr, attributes) {
                    m_editorInfo[attr.name().toString()] = attr.value().toString();
                }

                // Update fullnamespace-type of the scxmldocument
                if (m_tagType == Scxml) {
                    m_document->m_useFullNameSpace = m_editorInfo.value("fullnamespace", "false") == "true";
                    m_document->m_idDelimiter = m_editorInfo.value("fullnamespacedelimiter", "::");
                }

                xml.readNext();
            } else {
                ScxmlTag *childTag = nullptr;
                if ((m_tagType == Initial || m_tagType == History) && xml.name().toString() == "transition")
                    childTag = new ScxmlTag(InitialTransition, m_document);
                else
                    childTag = new ScxmlTag(xml.prefix().toString(),
                        xml.name().toString(), m_document);

                appendChild(childTag);
                childTag->readXml(xml, checkCopyId);
            }
        } else if (token == QXmlStreamReader::Invalid) {
            qDebug() << ScxmlTag::tr("Error in reading XML ") << xml.error() << ":"
                     << xml.errorString();
            break;
        }

        token = xml.readNext();
    }

    // Create initial state of the scxml (root)
    if (m_tagType == Scxml && !scxmlInitial.isEmpty()) {
        auto initialTag = new ScxmlTag(Initial, m_document);
        auto initialTransitionTag = new ScxmlTag(InitialTransition, m_document);
        initialTransitionTag->setAttribute("target", scxmlInitial);
        initialTag->setEditorInfo(Constants::C_SCXML_EDITORINFO_GEOMETRY,
            m_editorInfo.value("initialGeometry"));
        initialTransitionTag->setEditorInfo(
            Constants::C_SCXML_EDITORINFO_GEOMETRY, m_editorInfo.value("transitionGeometry"));

        appendChild(initialTag);
        initialTag->appendChild(initialTransitionTag);
    }
}
