/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "bardescriptordocument.h"

#include "qnxconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QMetaEnum>
#include <QTextCodec>
#include <QSet>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorDocument::BarDescriptorDocument(QObject *parent)
    : Core::TextDocument(parent)
{
    setId(Constants::QNX_BAR_DESCRIPTOR_EDITOR_ID);
    setMimeType(QLatin1String(Constants::QNX_BAR_DESCRIPTOR_MIME_TYPE));
    // blackberry-nativepackager requires the XML file to be in UTF-8 encoding,
    // force if possible
    if (QTextCodec *defaultUTF8 = QTextCodec::codecForName("UTF-8"))
        setCodec(defaultUTF8);
    else
        setCodec(Core::EditorManager::defaultTextCodec());
}

BarDescriptorDocument::~BarDescriptorDocument()
{
}

bool BarDescriptorDocument::open(QString *errorString, const QString &fileName) {
    QString contents;
    if (read(fileName, &contents, errorString) != Utils::TextFileFormat::ReadSuccess)
        return false;

    setFilePath(fileName);

    const bool result = loadContent(contents, false);

    if (!result)
        *errorString = tr("%1 does not appear to be a valid application descriptor file").arg(QDir::toNativeSeparators(fileName));

    return result;
}

bool BarDescriptorDocument::save(QString *errorString, const QString &fn, bool autoSave)
{
    QTC_ASSERT(!autoSave, return false);
    QTC_ASSERT(fn.isEmpty(), return false);

    const bool result = write(filePath(), xmlSource(), errorString);
    if (!result)
        return false;

    m_dirty = false;
    emit Core::IDocument::changed();
    return true;
}

QString BarDescriptorDocument::defaultPath() const
{
    QFileInfo fi(filePath());
    return fi.absolutePath();
}

QString BarDescriptorDocument::suggestedFileName() const
{
    QFileInfo fi(filePath());
    return fi.fileName();
}

bool BarDescriptorDocument::shouldAutoSave() const
{
    return false;
}

bool BarDescriptorDocument::isModified() const
{
    return m_dirty;
}

bool BarDescriptorDocument::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior BarDescriptorDocument::reloadBehavior(Core::IDocument::ChangeTrigger state, Core::IDocument::ChangeType type) const
{
    if (type == TypeRemoved || type == TypePermissions)
        return BehaviorSilent;
    if (type == TypeContents && state == TriggerInternal && !isModified())
        return BehaviorSilent;
    return BehaviorAsk;
}

bool BarDescriptorDocument::reload(QString *errorString, Core::IDocument::ReloadFlag flag, Core::IDocument::ChangeType type)
{
    Q_UNUSED(type);

    if (flag == Core::IDocument::FlagIgnore)
        return true;

    return open(errorString, filePath());
}

QString BarDescriptorDocument::xmlSource() const
{
    const int indent = 4;
    return m_barDocument.toString(indent);
}

bool BarDescriptorDocument::loadContent(const QString &xmlCode, bool setDirty, QString *errorMessage, int *errorLine)
{
    if (xmlCode == xmlSource())
        return true;

    bool result = m_barDocument.setContent(xmlCode, errorMessage, errorLine);

    m_dirty = setDirty;

    emitAllChanged();
    emit Core::IDocument::changed();
    return result;
}

QVariant BarDescriptorDocument::value(BarDescriptorDocument::Tag tag) const
{
    const QString tagName = QString::fromLatin1(metaObject()->enumerator(metaObject()->enumeratorOffset()).valueToKey(tag));

    switch (tag) {
    case id:
    case versionNumber:
    case buildId:
    case name:
    case description:
    case author:
    case publisher:
    case authorId:
        return stringValue(tagName);
    case icon:
        return childStringListValue(tagName, QLatin1String("image")).value(0);
    case splashScreens:
        return childStringListValue(tagName, QLatin1String("image"));
    case asset: {
        QVariant var;
        var.setValue(assets());
        return var;
    }
    case aspectRatio:
    case autoOrients:
    case systemChrome:
        return childStringListValue(QLatin1String("initialWindow"), tagName).value(0);
    case transparent:
        return childStringListValue(QLatin1String("initialWindow"), tagName).value(0) == QLatin1String("true");
    case arg:
    case action:
        return stringListValue(tagName);
    case env:
        QVariant var;
        var.setValue(environment());
        return var;
    }

    return QVariant();
}

void BarDescriptorDocument::setValue(BarDescriptorDocument::Tag tag, const QVariant &value)
{
    const QMetaEnum tagEnum = metaObject()->enumerator(metaObject()->enumeratorOffset());
    const QString tagName = QString::fromLatin1(tagEnum.valueToKey(tag));

    switch (tag) {
    case id:
    case versionNumber:
    case buildId:
    case name:
    case description:
    case authorId:
        setStringValue(tagName, value.toString());
        break;
    case icon:
    case splashScreens:
        setChildStringListValue(tagName, QLatin1String("image"), value.toStringList());
        break;
    case asset:
        setAssets(value.value<BarDescriptorAssetList>());
        break;
    case aspectRatio:
    case autoOrients:
    case systemChrome:
        setChildStringListValue(QLatin1String("initialWindow"), tagName, value.toStringList());
        break;
    case transparent:
        setChildStringListValue(QLatin1String("initialWindow"), tagName, QStringList() << (value.toBool() ? QLatin1String("true") : QLatin1String("false")));
        break;
    case arg:
    case action:
        setStringListValue(tagName, value.toStringList());
        break;
    case env:
        setEnvironment(value.value<QList<Utils::EnvironmentItem> >());
        break;
    case author:
    case publisher:
        // Unset <publisher> when setting <author> as only one should be used
        setStringValue(QString::fromLatin1(tagEnum.valueToKey(author)), value.toString());
        setStringValue(QString::fromLatin1(tagEnum.valueToKey(publisher)), QLatin1String(""));
        break;
    }

    m_dirty = true;
    emit changed(tag, value);
    emit Core::IDocument::changed();
}

QString BarDescriptorDocument::stringValue(const QString &tagName) const
{
    QDomNodeList nodes = m_barDocument.elementsByTagName(tagName);
    if (nodes.isEmpty() || nodes.size() > 1)
        return QString();

    QDomNode node = nodes.item(0);
    QDomText textNode = node.firstChild().toText();
    if (textNode.isNull())
        return QString();

    return textNode.data();
}

void BarDescriptorDocument::setStringValue(const QString &tagName, const QString &value)
{
    QDomNodeList nodes = m_barDocument.elementsByTagName(tagName);

    if (nodes.size() > 1)
        return;

    QDomNode existingNode = nodes.item(0);
    if (existingNode.isNull() && value.isEmpty())
        return;

    if (!existingNode.isNull() && value.isEmpty()) {
        m_barDocument.documentElement().removeChild(existingNode);
    } else if (existingNode.isNull()) {
        QDomElement newNode = m_barDocument.createElement(tagName);
        newNode.appendChild(m_barDocument.createTextNode(value));
        m_barDocument.documentElement().appendChild(newNode);
    } else {
        QDomText textNode = existingNode.firstChild().toText();
        if (textNode.isNull())
            return;
        textNode.setData(value);
    }
}

QStringList BarDescriptorDocument::childStringListValue(const QString &tagName, const QString &childTagName) const
{
    QDomNodeList nodes = m_barDocument.elementsByTagName(tagName);
    if (nodes.isEmpty() || nodes.size() > 1)
        return QStringList();

    QDomNode parentNode = nodes.item(0);
    QDomElement childElm = parentNode.firstChildElement(childTagName);
    if (childElm.isNull())
        return QStringList();

    QStringList result;
    while (!childElm.isNull()) {
        QDomText textNode = childElm.firstChild().toText();
        if (textNode.isNull())
            return QStringList();

        result.append(textNode.data());

        childElm = childElm.nextSiblingElement(childTagName);
    }

    return result;
}

void BarDescriptorDocument::setChildStringListValue(const QString &tagName, const QString &childTagName, const QStringList &stringList)
{
    QDomNodeList nodes = m_barDocument.elementsByTagName(tagName);

    if (nodes.size() > 1)
        return;

    QDomNode existingNode = nodes.item(0);

    if (existingNode.isNull()) {
        QDomElement newParentNode = m_barDocument.createElement(tagName);

        foreach (const QString &value, stringList) {
            QDomElement newChildNode = m_barDocument.createElement(childTagName);
            QDomText newTextNode = m_barDocument.createTextNode(value);
            newChildNode.appendChild(newTextNode);
            newParentNode.appendChild(newChildNode);
        }
        m_barDocument.documentElement().appendChild(newParentNode);
    } else {
        QStringList values = stringList;
        QDomElement childElm = existingNode.firstChildElement(childTagName);
        if (!childElm.isNull()) {
            // Loop through existing elements, remove the existing nodes
            // that no longer are in "values", and remove from "values"
            // the existing nodes that don't need re-creation
            while (!childElm.isNull()) {
                QDomText textNode = childElm.firstChild().toText();
                if (textNode.isNull())
                    continue;

                QDomElement toRemove;
                if (!values.contains(textNode.data()))
                    toRemove = childElm;
                else
                    values.removeAll(textNode.data());

                childElm = childElm.nextSiblingElement(childTagName);

                if (!toRemove.isNull())
                    existingNode.removeChild(toRemove);
            }
        }

        // Add the new elements
        int newElementCount = 0;
        foreach (const QString &value, values) {
            if (value.isEmpty())
                continue;
            QDomElement newChildNode = m_barDocument.createElement(childTagName);
            newChildNode.appendChild(m_barDocument.createTextNode(value));
            existingNode.appendChild(newChildNode);
            ++newElementCount;
        }

        if (newElementCount == 0)
            m_barDocument.documentElement().removeChild(existingNode);
    }
}

QStringList BarDescriptorDocument::stringListValue(const QString &tagName) const
{
    QStringList result;

    QDomElement childElm = m_barDocument.documentElement().firstChildElement(tagName);
    while (!childElm.isNull()) {
        QDomText textNode = childElm.firstChild().toText();
        if (textNode.isNull())
            continue;

        result.append(textNode.data());

        childElm = childElm.nextSiblingElement(tagName);
    }

    return result;
}

void BarDescriptorDocument::setStringListValue(const QString &tagName, const QStringList &stringList)
{
    QStringList values = stringList;
    QDomElement childElm = m_barDocument.documentElement().firstChildElement(tagName);
    if (!childElm.isNull()) {
        // Loop through existing elements, remove the existing nodes
        // that no longer are in "values", and remove from "values"
        // the existing nodes that don't need re-creation
        while (!childElm.isNull()) {
            QDomText textNode = childElm.firstChild().toText();
            if (textNode.isNull())
                continue;

            QDomElement toRemove;
            if (!values.contains(textNode.data()))
                toRemove = childElm;
            else
                values.removeAll(textNode.data());

            childElm = childElm.nextSiblingElement(tagName);

            if (!toRemove.isNull())
                m_barDocument.documentElement().removeChild(toRemove);
        }
    }

    // Add the new elements
    foreach (const QString &value, values) {
        if (value.isEmpty())
            continue;
        QDomElement newChildNode = m_barDocument.createElement(tagName);
        newChildNode.appendChild(m_barDocument.createTextNode(value));
        m_barDocument.documentElement().appendChild(newChildNode);
    }
}

BarDescriptorAssetList BarDescriptorDocument::assets() const
{
    BarDescriptorAssetList result;
    QDomNodeList nodes = m_barDocument.elementsByTagName(QLatin1String("asset"));
    if (nodes.isEmpty())
        return result;

    for (int i = 0; i < nodes.size(); ++i) {
        QDomElement assetElm = nodes.item(i).toElement();
        if (assetElm.isNull())
            continue;

        QDomText textNode = assetElm.firstChild().toText();
        if (textNode.isNull())
            continue;

        QString path = assetElm.attribute(QLatin1String("path"));
        QString entry = assetElm.attribute(QLatin1String("entry"));
        QString dest = textNode.data();

        BarDescriptorAsset asset;
        asset.source = path;
        asset.destination = dest;
        asset.entry = entry == QLatin1String("true");
        result.append(asset);
    }

    return result;
}

void BarDescriptorDocument::setAssets(const BarDescriptorAssetList &assets)
{
    QDomNodeList nodes = m_barDocument.elementsByTagName(QLatin1String("asset"));

    BarDescriptorAssetList newAssets = assets;
    QList<QDomNode> toRemove;

    for (int i = 0; i < nodes.size(); ++i) {
        QDomElement assetElm = nodes.at(i).toElement();
        if (assetElm.isNull())
            continue;

        QDomText textNode = assetElm.firstChild().toText();
        if (textNode.isNull())
            continue;

        QString source = assetElm.attribute(QLatin1String("path"));
        bool found = false;
        foreach (const BarDescriptorAsset &asset, newAssets) {
            if (asset.source == source) {
                found = true;
                if (asset.entry) {
                    assetElm.setAttribute(QLatin1String("type"), QLatin1String("Qnx/Elf"));
                    assetElm.setAttribute(QLatin1String("entry"), QLatin1String("true"));
                } else {
                    assetElm.removeAttribute(QLatin1String("type"));
                    assetElm.removeAttribute(QLatin1String("entry"));
                }
                textNode.setData(asset.destination);

                newAssets.removeAll(asset);
                break;
            }
        }

        if (!found)
            toRemove.append(assetElm);
    }

    foreach (const QDomNode &node, toRemove)
        m_barDocument.documentElement().removeChild(node);

    foreach (const BarDescriptorAsset &asset, newAssets) {
        QDomElement assetElm = m_barDocument.createElement(QLatin1String("asset"));
        assetElm.setAttribute(QLatin1String("path"), asset.source);
        if (asset.entry) {
            assetElm.setAttribute(QLatin1String("type"), QLatin1String("Qnx/Elf"));
            assetElm.setAttribute(QLatin1String("entry"), QLatin1String("true"));
        }
        assetElm.appendChild(m_barDocument.createTextNode(asset.destination));
        m_barDocument.documentElement().appendChild(assetElm);
    }
}

QList<Utils::EnvironmentItem> BarDescriptorDocument::environment() const
{
    QList<Utils::EnvironmentItem> result;

    QDomElement envElm = m_barDocument.documentElement().firstChildElement(QLatin1String("env"));
    while (!envElm.isNull()) {
        QString var = envElm.attribute(QLatin1String("var"));
        QString value = envElm.attribute(QLatin1String("value"));

        Utils::EnvironmentItem item(var, value);
        result.append(item);

        envElm = envElm.nextSiblingElement(QLatin1String("env"));
    }
    return result;
}

void BarDescriptorDocument::setEnvironment(const QList<Utils::EnvironmentItem> &environment)
{
    QDomNodeList envNodes = m_barDocument.elementsByTagName(QLatin1String("env"));

    QList<Utils::EnvironmentItem> newEnvironment = environment;
    QList<QDomElement> toRemove;
    for (int i = 0; i < envNodes.size(); ++i) {
        QDomElement elm = envNodes.at(i).toElement();
        if (elm.isNull())
            continue;

        QString var = elm.attribute(QLatin1String("var"));
        bool found = false;
        foreach (const Utils::EnvironmentItem item, newEnvironment) {
            if (item.name == var) {
                found = true;
                elm.setAttribute(QLatin1String("value"), item.value);
                newEnvironment.removeAll(item);
                break;
            }
        }

        if (!found)
            toRemove.append(elm);
    }

    foreach (const QDomNode &node, toRemove)
        m_barDocument.documentElement().removeChild(node);

    foreach (const Utils::EnvironmentItem item, newEnvironment) {
        QDomElement elm = m_barDocument.createElement(QLatin1String("env"));
        elm.setAttribute(QLatin1String("var"), item.name);
        elm.setAttribute(QLatin1String("value"), item.value);
        m_barDocument.documentElement().appendChild(elm);
    }
}

void BarDescriptorDocument::emitAllChanged()
{
    QMetaEnum tags = metaObject()->enumerator(metaObject()->enumeratorOffset());
    for (int i = 0; i < tags.keyCount(); ++i) {
        Tag tag = static_cast<Tag>(tags.value(i));
        emit changed(tag, value(tag));
    }
}

QString BarDescriptorDocument::bannerComment() const
{
    QDomNode nd = m_barDocument.firstChild();
    QDomProcessingInstruction pi = nd.toProcessingInstruction();
    if (!pi.isNull())
        nd = pi.nextSibling();

    return nd.toComment().data();
}

void BarDescriptorDocument::setBannerComment(const QString &commentText)
{
    QDomNode nd = m_barDocument.firstChild();
    QDomProcessingInstruction pi = nd.toProcessingInstruction();
    if (!pi.isNull())
        nd = pi.nextSibling();

    bool oldDirty = m_dirty;
    QDomComment cnd = nd.toComment();
    if (cnd.isNull()) {
        if (!commentText.isEmpty()) {
            cnd = m_barDocument.createComment(commentText);
            m_barDocument.insertBefore(cnd, nd);
            m_dirty = true;
        }
    } else {
        if (commentText.isEmpty()) {
            m_barDocument.removeChild(cnd);
            m_dirty = true;
        } else {
            if (cnd.data() != commentText) {
                cnd.setData(commentText);
                m_dirty = true;
            }
        }
    }
    if (m_dirty != oldDirty)
        emit Core::IDocument::changed();
}

int BarDescriptorDocument::tagForElement(const QDomElement &element)
{
    QMetaEnum tags = metaObject()->enumerator(metaObject()->enumeratorOffset());
    QDomElement el = element;
    while (!el.isNull()) {
        const int n = tags.keyToValue(el.tagName().toLatin1().constData());
        if (n > -1)
            return n;
        el = el.parentNode().toElement();
    }
    return -1;
}

bool BarDescriptorDocument::expandPlaceHolder_helper(const QDomElement &el,
                                                     const QString &placeholderKey,
                                                     const QString &placeholderText,
                                                     QSet<BarDescriptorDocument::Tag> &changedTags)
{
    // replace attributes
    bool elementChanged = false;
    QDomNamedNodeMap attrs = el.attributes();
    for (int i = 0; i < attrs.count(); ++i) {
        QDomAttr attr = attrs.item(i).toAttr();
        if (!attr.isNull()) {
            QString s = attr.value();
            s.replace(placeholderKey, placeholderText);
            if (s != attr.value()) {
                attr.setValue(s);
                elementChanged = true;
            }
        }
    }

    bool documentChanged = false;
    // replace text
    for (QDomNode nd = el.firstChild(); !nd.isNull(); nd = nd.nextSibling()) {
        QDomText txtnd = nd.toText();
        if (!txtnd.isNull()) {
            QString s = txtnd.data();
            s.replace(placeholderKey, placeholderText);
            if (s != txtnd.data()) {
                txtnd.setData(s);
                elementChanged = true;
            }
        }
        QDomElement child = nd.toElement();
        if (!child.isNull()) {
            bool hit = expandPlaceHolder_helper(child, placeholderKey, placeholderText, changedTags);
            documentChanged = documentChanged || hit;
        }
    }
    if (elementChanged) {
        int n = tagForElement(el);
        if (n >= 0)
            changedTags << static_cast<Tag>(n);
    }
    documentChanged = documentChanged || elementChanged;
    return documentChanged;
}

void BarDescriptorDocument::expandPlaceHolders(const QHash<QString, QString> &placeholdersKeyVals)
{
    QSet<Tag> changedTags;
    QHashIterator<QString, QString> it(placeholdersKeyVals);
    bool docChanged = false;
    while (it.hasNext()) {
        it.next();
        bool expanded = expandPlaceHolder_helper(m_barDocument.documentElement(),
                                                 it.key(), it.value(), changedTags);
        docChanged = docChanged || expanded;
    }
    m_dirty = m_dirty || docChanged;
    foreach (Tag tag, changedTags)
        emit changed(tag, value(tag));
    if (docChanged)
        emit Core::IDocument::changed();
}
