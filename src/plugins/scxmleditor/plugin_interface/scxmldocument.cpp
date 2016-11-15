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

#include "scxmldocument.h"
#include "scxmlnamespace.h"
#include "scxmltagutils.h"
#include "undocommands.h"

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <app/app_version.h>

using namespace ScxmlEditor::PluginInterface;

ScxmlDocument::ScxmlDocument(const QString &fileName, QObject *parent)
    : QObject(parent)
{
    initVariables();
    m_fileName = fileName;
    load(fileName);
}

ScxmlDocument::ScxmlDocument(const QByteArray &data, QObject *parent)
    : QObject(parent)
{
    initVariables();
    load(QLatin1String(data));
}

ScxmlDocument::~ScxmlDocument()
{
    clear(false);
}

void ScxmlDocument::initVariables()
{
    m_idDelimiter = "::";
    m_undoStack = new QUndoStack(this);
    connect(m_undoStack, &QUndoStack::cleanChanged, this, &ScxmlDocument::documentChanged);
}

void ScxmlDocument::clear(bool createRoot)
{
    m_currentTag = nullptr;
    m_nextIdHash.clear();

    // First clear undostack
    m_undoStack->clear();

    // Second delete all other available tags
    // tags will call the removeChild-function -> m_tags will be cleared
    for (int i = m_tags.count(); i--;)
        delete m_tags[i];

    m_rootTags.clear();
    clearNamespaces();

    if (createRoot) {
        pushRootTag(createScxmlTag());
        rootTag()->setAttribute("qt:editorversion", QLatin1String(Core::Constants::IDE_VERSION_LONG));

        auto ns = new ScxmlNamespace("qt", "http://www.qt.io/2015/02/scxml-ext");
        ns->setTagVisibility("editorInfo", false);
        addNamespace(ns);
    }

    m_useFullNameSpace = false;
}

QString ScxmlDocument::nameSpaceDelimiter() const
{
    return m_idDelimiter;
}

bool ScxmlDocument::useFullNameSpace() const
{
    return m_useFullNameSpace;
}

void ScxmlDocument::setNameSpaceDelimiter(const QString &delimiter)
{
    m_idDelimiter = delimiter;
}

QString ScxmlDocument::fileName() const
{
    return m_fileName;
}

void ScxmlDocument::setFileName(const QString &filename)
{
    m_fileName = filename;
}

ScxmlNamespace *ScxmlDocument::scxmlNamespace(const QString &prefix)
{
    return m_namespaces.value(prefix, 0);
}

void ScxmlDocument::addNamespace(ScxmlNamespace *ns)
{
    if (!ns)
        return;

    delete m_namespaces.take(ns->prefix());
    m_namespaces[ns->prefix()] = ns;

    ScxmlTag *scxmlTag = scxmlRootTag();
    if (scxmlTag) {
        QMapIterator<QString, ScxmlNamespace*> i(m_namespaces);
        while (i.hasNext()) {
            i.next();
            QString prefix = i.value()->prefix();
            if (prefix.isEmpty())
                prefix = "xmlns";

            if (prefix.startsWith("xmlns"))
                scxmlTag->setAttribute(prefix, i.value()->name());
            else
                scxmlTag->setAttribute(QString::fromLatin1("xmlns:%1").arg(prefix), i.value()->name());
        }
    }
}

void ScxmlDocument::clearNamespaces()
{
    while (!m_namespaces.isEmpty()) {
        delete m_namespaces.take(m_namespaces.firstKey());
    }
}

bool ScxmlDocument::generateSCXML(QIODevice *io, ScxmlTag *tag) const
{
    QXmlStreamWriter xml(io);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    if (tag)
        tag->writeXml(xml);
    else
        rootTag()->writeXml(xml);
    xml.writeEndDocument();

    return !xml.hasError();
}

ScxmlTag *ScxmlDocument::createScxmlTag()
{
    auto tag = new ScxmlTag(Scxml, this);
    QMapIterator<QString, ScxmlNamespace*> i(m_namespaces);
    while (i.hasNext()) {
        i.next();
        QString prefix = i.value()->prefix();
        if (prefix.isEmpty())
            prefix = "xmlns";

        if (prefix.startsWith("xmlns"))
            tag->setAttribute(prefix, i.value()->name());
        else
            tag->setAttribute(QString::fromLatin1("xmlns:%1").arg(prefix), i.value()->name());
    }
    return tag;
}

bool ScxmlDocument::hasLayouted() const
{
    return m_hasLayouted;
}
QColor ScxmlDocument::getColor(int depth) const
{
    return m_colors.isEmpty() ? QColor(Qt::gray) : m_colors[depth % m_colors.count()];
}

void ScxmlDocument::setLevelColors(const QVector<QColor> &colors)
{
    m_colors = colors;
    emit colorThemeChanged();
}

bool ScxmlDocument::load(QIODevice *io)
{
    m_currentTag = nullptr;
    clearNamespaces();

    bool ok = true;
    clear(false);

    QXmlStreamReader xml(io);
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartDocument)
            continue;

        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "scxml") {
                // Get and add namespaces
                QXmlStreamNamespaceDeclarations ns = xml.namespaceDeclarations();
                for (int i = 0; i < ns.count(); ++i)
                    addNamespace(new ScxmlNamespace(ns[i].prefix().toString(), ns[i].namespaceUri().toString()));

                // create root tag
                pushRootTag(createScxmlTag());

                // and read other tags also
                rootTag()->readXml(xml);

                // Check editorversion
                m_hasLayouted = rootTag()->hasAttribute("qt:editorversion");
                rootTag()->setAttribute("qt:editorversion", QLatin1String(Core::Constants::IDE_VERSION_LONG));
            }
        }

        if (token == QXmlStreamReader::Invalid)
            break;
    }

    if (xml.hasError()) {
        m_hasError = true;
        initErrorMessage(xml, io);
        m_fileName.clear();
        ok = false;

        clear();
    } else {
        m_hasError = false;
        m_lastError.clear();
    }

    m_undoStack->setClean();

    return ok;
}

void ScxmlDocument::initErrorMessage(const QXmlStreamReader &xml, QIODevice *io)
{
    QString errorString;
    switch (xml.error()) {
    case QXmlStreamReader::Error::UnexpectedElementError:
        errorString = tr("Unexpected element");
        break;
    case QXmlStreamReader::Error::NotWellFormedError:
        errorString = tr("Not well formed");
        break;
    case QXmlStreamReader::Error::PrematureEndOfDocumentError:
        errorString = tr("Premature end of document");
        break;
    case QXmlStreamReader::Error::CustomError:
        errorString = tr("Custom error");
        break;
    default:
        break;
    }

    QString lineString;
    io->seek(0);
    for (int i = 0; i < xml.lineNumber() - 1; ++i)
        io->readLine();
    lineString = QLatin1String(io->readLine());

    m_lastError = tr("Error in reading XML.\nType: %1 (%2)\nDescription: %3\n\nRow: %4, Column: %5\n%6")
                      .arg(xml.error())
                      .arg(errorString)
                      .arg(xml.errorString())
                      .arg(xml.lineNumber())
                      .arg(xml.columnNumber())
                      .arg(lineString);
}

bool ScxmlDocument::pasteData(const QByteArray &data, const QPointF &minPos, const QPointF &pastePos)
{
    if (!m_currentTag)
        m_currentTag = rootTag();

    if (!m_currentTag) {
        m_hasError = true;
        m_lastError = tr("Current tag not selected");
        return false;
    }

    if (data.trimmed().isEmpty()) {
        m_hasError = true;
        m_lastError = tr("Pasted data is empty.");
        return false;
    }

    bool ok = true;
    m_undoStack->beginMacro(tr("Paste item(s)"));

    QByteArray d(data);
    QBuffer buffer(&d);
    buffer.open(QIODevice::ReadOnly);
    QXmlStreamReader xml(&buffer);
    foreach (ScxmlNamespace *ns, m_namespaces) {
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration(ns->prefix(), ns->name()));
    }

    m_idMap.clear();
    QVector<ScxmlTag*> addedTags;

    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartDocument)
            continue;

        if (token == QXmlStreamReader::StartElement) {
            if (xml.name().toString() == "scxml")
                continue;

            ScxmlTag *childTag = nullptr;
            if ((m_currentTag->tagType() == Initial || m_currentTag->tagType() == History) && xml.name().toString() == "transition")
                childTag = new ScxmlTag(InitialTransition, this);
            else
                childTag = new ScxmlTag(xml.prefix().toString(), xml.name().toString(), this);

            childTag->readXml(xml, true);
            addedTags << childTag;
        }

        if (token == QXmlStreamReader::Invalid)
            break;
    }

    if (xml.error()) {
        m_hasError = true;
        qDeleteAll(addedTags);
        addedTags.clear();
        initErrorMessage(xml, &buffer);
        ok = false;
    } else {
        m_hasError = false;
        m_lastError.clear();

        // Fine-tune names and coordinates
        for (int i = 0; i < addedTags.count(); ++i)
            TagUtils::modifyPosition(addedTags[i], minPos, pastePos);

        // Update targets and initial-attributes
        for (int i = 0; i < addedTags.count(); ++i)
            addedTags[i]->finalizeTagNames();

        // Add tags to the document
        addTags(m_currentTag, addedTags);
    }

    m_undoStack->endMacro();

    return ok;
}

void ScxmlDocument::load(const QString &fileName)
{
    if (QFile::exists(fileName)) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (load(&file)) {
                m_fileName = fileName;
            }
        }
    }

    // If loading doesn't work, create root tag here
    if (m_rootTags.isEmpty()) {
        pushRootTag(createScxmlTag());
        rootTag()->setAttribute("qt:editorversion", QLatin1String(Core::Constants::IDE_VERSION_LONG));
    }

    auto ns = new ScxmlNamespace("qt", "http://www.qt.io/2015/02/scxml-ext");
    ns->setTagVisibility("editorInfo", false);
    addNamespace(ns);
}

void ScxmlDocument::printSCXML()
{
    qDebug() << content();
}

QByteArray ScxmlDocument::content(const QVector<ScxmlTag*> &tags) const
{
    QByteArray result;
    if (tags.count() > 0) {
        QBuffer buffer(&result);
        buffer.open(QIODevice::WriteOnly);

        bool writeScxml = tags.count() > 1 || tags[0]->tagType() != Scxml;

        QXmlStreamWriter xml(&buffer);
        xml.setAutoFormatting(true);
        xml.writeStartDocument();
        if (writeScxml)
            xml.writeStartElement("scxml");

        foreach (ScxmlTag *tag, tags) {
            tag->writeXml(xml);
        }
        xml.writeEndDocument();

        if (writeScxml)
            xml.writeEndElement();
    }
    return result;
}

QByteArray ScxmlDocument::content(ScxmlTag *tag) const
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    generateSCXML(&buffer, tag);
    return byteArray;
}

bool ScxmlDocument::save()
{
    return save(m_fileName);
}

bool ScxmlDocument::save(const QString &fileName)
{
    QString name(fileName);
    if (!name.endsWith(".scxml", Qt::CaseInsensitive))
        name.append(".scxml");

    bool ok = true;

    QFile file(name);
    if (file.open(QIODevice::WriteOnly)) {
        ok = generateSCXML(&file, scxmlRootTag());
        if (ok) {
            m_fileName = name;
            m_undoStack->setClean();
        }
        file.close();
        if (!ok)
            m_lastError = tr("Cannot save xml to the file %1.").arg(fileName);
    } else {
        ok = false;
        m_lastError = tr("Cannot open file %1.").arg(fileName);
    }

    return ok;
}

void ScxmlDocument::setContent(ScxmlTag *tag, const QString &content)
{
    if (tag && !m_undoRedoRunning)
        m_undoStack->push(new SetContentCommand(this, tag, content));
}

void ScxmlDocument::setValue(ScxmlTag *tag, const QString &key, const QString &value)
{
    if (tag && !m_undoRedoRunning)
        m_undoStack->push(new SetAttributeCommand(this, tag, key, value));
}

void ScxmlDocument::setValue(ScxmlTag *tag, int attributeIndex, const QString &value)
{
    if (tag && attributeIndex >= 0 && attributeIndex < tag->info()->n_attributes)
        m_undoStack->push(new SetAttributeCommand(this, tag, QLatin1String(tag->info()->attributes[attributeIndex].name), value));
}

void ScxmlDocument::setEditorInfo(ScxmlTag *tag, const QString &key, const QString &value)
{
    if (tag && !m_undoRedoRunning)
        m_undoStack->push(new SetEditorInfoCommand(this, tag, key, value));
}

void ScxmlDocument::setCurrentTag(ScxmlTag *tag)
{
    if (tag != m_currentTag) {
        emit beginTagChange(TagCurrentChanged, tag, QVariant());
        m_currentTag = tag;
        emit endTagChange(TagCurrentChanged, tag, QVariant());
    }
}

ScxmlTag *ScxmlDocument::currentTag() const
{
    return m_currentTag;
}

void ScxmlDocument::changeParent(ScxmlTag *child, ScxmlTag *newParent, int tagIndex)
{
    if (child && child->parentTag() != newParent && !m_undoRedoRunning)
        m_undoStack->push(new ChangeParentCommand(this, child, !newParent ? rootTag() : newParent, tagIndex));
}

void ScxmlDocument::changeOrder(ScxmlTag *child, int newPos)
{
    if (child && !m_undoRedoRunning) {
        ScxmlTag *parentTag = child->parentTag();
        if (parentTag) {
            m_undoStack->push(new ChangeOrderCommand(this, child, parentTag, newPos));
        }
    }
}

void ScxmlDocument::addTags(ScxmlTag *parent, const QVector<ScxmlTag*> tags)
{
    if (m_undoRedoRunning)
        return;

    if (!parent)
        parent = rootTag();

    m_undoStack->push(new AddRemoveTagsBeginCommand(this, parent));
    for (int i = 0; i < tags.count(); ++i)
        addTag(parent, tags[i]);
    m_undoStack->push(new AddRemoveTagsEndCommand(this, parent));
}

void ScxmlDocument::addTag(ScxmlTag *parent, ScxmlTag *child)
{
    if (m_undoRedoRunning)
        return;

    if (!parent)
        parent = rootTag();

    if (parent && child) {
        m_undoStack->beginMacro(tr("Add Tag"));
        addTagRecursive(parent, child);
        m_undoStack->endMacro();
    }
}

void ScxmlDocument::removeTag(ScxmlTag *tag)
{
    if (tag && !m_undoRedoRunning) {
        // Create undo/redo -macro, because state can includes lot of child-states
        m_undoStack->beginMacro(tr("Remove Tag"));
        removeTagRecursive(tag);
        m_undoStack->endMacro();
    }
}

void ScxmlDocument::addTagRecursive(ScxmlTag *parent, ScxmlTag *tag)
{
    if (tag && !m_undoRedoRunning) {
        m_undoStack->push(new AddRemoveTagCommand(this, parent, tag, TagAddChild));

        // First create AddRemoveTagCommands for the all children recursive
        for (int i = 0; i < tag->childCount(); ++i)
            addTagRecursive(tag, tag->child(i));
    }
}

void ScxmlDocument::removeTagRecursive(ScxmlTag *tag)
{
    if (tag && !m_undoRedoRunning) {
        // First create AddRemoveTagCommands for the all children recursive
        int childCount = tag->childCount();
        for (int i = childCount; i--;)
            removeTagRecursive(tag->child(i));

        m_undoStack->push(new AddRemoveTagCommand(this, tag->parentTag(), tag, TagRemoveChild));
    }
}

void ScxmlDocument::setUseFullNameSpace(bool use)
{
    if (m_useFullNameSpace != use)
        m_undoStack->push(new ChangeFullNameSpaceCommand(this, scxmlRootTag(), use));
}

QString ScxmlDocument::nextUniqueId(const QString &key)
{
    QString name;
    bool bFound = false;
    int id = 0;
    while (true) {
        id = m_nextIdHash.value(key, 0) + 1;
        m_nextIdHash[key] = id;
        bFound = false;
        name = QString::fromLatin1("%1_%2").arg(key).arg(id);

        // Check duplicate
        foreach (const ScxmlTag *tag, m_tags) {
            if (tag->attribute("id") == name) {
                bFound = true;
                break;
            }
        }
        if (!bFound)
            break;
    }
    return name;
}

QString ScxmlDocument::getUniqueCopyId(const ScxmlTag *tag)
{
    const QString key = tag->attribute("id");
    QString name = key;
    int counter = 1;
    bool bFound = false;

    while (true) {
        bFound = false;
        // Check duplicate
        foreach (const ScxmlTag *t, m_tags) {
            if (t->attribute("id") == name && t != tag) {
                name = QString::fromLatin1("%1_Copy(%2)").arg(key).arg(counter);
                bFound = true;
                counter++;
            }
        }

        if (!bFound)
            break;
    }

    return name;
}

bool ScxmlDocument::changed() const
{
    return !m_undoStack->isClean();
}

ScxmlTag *ScxmlDocument::scxmlRootTag() const
{
    ScxmlTag *tag = rootTag();
    while (tag && tag->tagType() != Scxml) {
        tag = tag->parentTag();
    }

    return tag;
}

ScxmlTag *ScxmlDocument::rootTag() const
{
    return m_rootTags.isEmpty() ? 0 : m_rootTags.last();
}

void ScxmlDocument::pushRootTag(ScxmlTag *tag)
{
    m_rootTags << tag;
}

ScxmlTag *ScxmlDocument::popRootTag()
{
    return m_rootTags.takeLast();
}

void ScxmlDocument::deleteRootTags()
{
    while (m_rootTags.count() > 0)
        delete m_rootTags.takeLast();
}

QUndoStack *ScxmlDocument::undoStack() const
{
    return m_undoStack;
}

void ScxmlDocument::addChild(ScxmlTag *tag)
{
    if (!m_tags.contains(tag))
        m_tags << tag;
}

void ScxmlDocument::removeChild(ScxmlTag *tag)
{
    m_tags.removeAll(tag);
}

void ScxmlDocument::setUndoRedoRunning(bool para)
{
    m_undoRedoRunning = para;
}

QFileInfo ScxmlDocument::qtBinDir() const
{
    return m_qtBinDir;
}
