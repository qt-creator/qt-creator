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

#pragma once

#include "scxmltypes.h"
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlDocument;

/**
 * @brief The ScxmlTag class represents one element in the SCXML document tree.
 *
 * Tag have a tagType, info and zero or more attributes associated with them. TagType must be given in the constructor and
 * it is not possible to change it. You can get and set attributes with the functions attribute(), setAttribute() and hasAttribute().
 *
 * ScxmlTag has one parent-tag and zero or more child. Tag doesn't take ownerships of the children.
 * It is user responsibility to delete all tags. Normally this will be done via the ScxmlDocument inside the undo-commands.
 */
class ScxmlTag : public QObject
{
    Q_OBJECT
public:
    ScxmlTag(TagType type, ScxmlDocument *document = nullptr);
    ScxmlTag(const QString &prefix, const QString &name, ScxmlDocument *document = nullptr);
    ScxmlTag(const ScxmlTag *other, bool copyChildren = true);

    ~ScxmlTag() override;

    void setDocument(ScxmlDocument *document);
    ScxmlDocument *document() const;

    // Functions for handling tagType/tagName
    TagType tagType() const;
    QString prefix() const;
    QString tagName(bool addPrefix = true) const;
    void setTagName(const QString &name);
    const scxmltag_type_t *info() const;
    QString displayName() const;
    QString stateNameSpace() const;

    // Get/set content
    QString content() const;
    void setContent(const QString &content);

    // Handling editorInfo
    void setEditorInfo(const QString &key, const QString &value);
    QString editorInfo(const QString &key) const;
    bool hasEditorInfo(const QString &key) const;

    // Functions for handling attributes
    int attributeCount() const;
    QStringList attributeNames() const;
    QStringList attributeValues() const;
    void setAttributeName(int ind, const QString &name);
    void setAttribute(int ind, const QString &name);
    QString attributeName(int ind) const;
    QString attribute(const QString &attribute, bool useNameSpace = false, const QString &defaultValue = QString()) const;
    QString attribute(int ind, const QString &defaultValue = QString()) const;
    void setAttribute(const QString &attribute, const QString &value);
    bool hasAttribute(const QString &key) const;

    // Functions for handling parent
    void setParentTag(ScxmlTag *parentTag);
    ScxmlTag *parentTag() const;
    bool hasParentTag() const;

    // Functions for handling child tags
    bool hasData() const;
    bool hasChild(TagType type) const;
    bool hasChild(const QString &name) const;
    void insertChild(int index, ScxmlTag *child);
    void appendChild(ScxmlTag *child);
    void removeChild(ScxmlTag *child);
    void moveChild(int oldPos, int newPos);
    int childCount() const;
    QVector<ScxmlTag*> allChildren() const;
    QVector<ScxmlTag*> children(const QString &name) const;
    ScxmlTag *child(const QString &name) const;
    ScxmlTag *child(int row) const;
    int childIndex(const ScxmlTag *child) const;
    int index() const;
    bool isRootTag() const;

    /**
     * @brief writeXml - write tag's content with the QXMLStreamWriter. Call writeXml-function for all children too.
     * @param xml
     */
    void writeXml(QXmlStreamWriter &xml);

    /**
     * @brief readXml - read tag's information from the QXMLStreamReader and create child tags if necessary.
     * @param xml
     */
    void readXml(QXmlStreamReader &xml, bool checkCopyId = false);

    void finalizeTagNames();
    void print();

private:
    void init(TagType type);
    void initId();

    const scxmltag_type_t *m_info = nullptr;
    QStringList m_attributeNames;
    QStringList m_attributeValues;
    QPointer<ScxmlTag> m_parentTag;
    QVector<ScxmlTag*> m_childTags;
    QPointer<ScxmlDocument> m_document;
    TagType m_tagType = UnknownTag;
    QString m_tagName;
    QString m_content;
    QString m_prefix;
    QHash<QString, QString> m_editorInfo;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
