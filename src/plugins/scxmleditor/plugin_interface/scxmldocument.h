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

#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QUndoStack)
QT_FORWARD_DECLARE_CLASS(QXmlStreamReader)

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlNamespace;
class ScxmlTag;

/**
 * @brief The ScxmlDocument class represents an SCXML document.
 *
 * This is the root of the document tree, and provides the access to the document's data.
 * The tag-element is ScxmlTag. You can create ScxmlTag-objects outside this class, but you have to call data-manipulation
 * functions from this class to be sure that all views can update their tags. When calling data-manipulation functions, this class
 * will send beginTagChange and endTagChange signals with the correspond values. This class also keep the undo-stack of the changes.
 *
 * When document has changed, the signal documentChanged(bool) will be emitted.
 */
class ScxmlDocument : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief The TagChange enum
     */
    enum TagChange {
        TagAddChild = 0,
        TagAddTags,
        TagRemoveChild,
        TagRemoveTags,
        TagCurrentChanged,
        TagAttributesChanged,
        TagEditorInfoChanged,
        TagChangeParent,
        TagChangeParentRemoveChild,
        TagChangeParentAddChild,
        TagChangeOrder,
        TagContentChanged,
        TagChangeFullNameSpace
    };

    explicit ScxmlDocument(const QString &fileName = QString(), QObject *parent = nullptr);
    explicit ScxmlDocument(const QByteArray &data, QObject *parent = nullptr);
    ~ScxmlDocument() override;

    void addNamespace(ScxmlNamespace *ns);
    ScxmlNamespace *scxmlNamespace(const QString &prefix);

    /**
     * @brief setCurrentTag - inform views that current selected tag has changed
     * @param tag - current tag
     */
    void setCurrentTag(ScxmlTag *tag);
    ScxmlTag *currentTag() const;

    /**
     * @brief setContent - inform views that tag-content has changed
     * @param tag
     * @param content
     */
    void setContent(ScxmlTag *tag, const QString &content);

    /**
     * @brief setValue - inform views that tag-value has changed
     * @param tag
     * @param key
     * @param value
     */
    void setValue(ScxmlTag *tag, const QString &key, const QString &value);

    /**
     * @brief setValue - inform views that tag-value has changed
     * @param tag
     * @param attributeIndex
     * @param value
     */
    void setValue(ScxmlTag *tag, int attributeIndex, const QString &value);

    /**
     * @brief setEditorInfo - inform views (and save undo-command) that editorinfo has changed
     * @param tag
     * @param key
     * @param value
     */
    void setEditorInfo(ScxmlTag *tag, const QString &key, const QString &value);

    /**
     * @brief addTag - inform views that new child will be added to the tree
     * @param parent
     * @param child
     */
    void addTag(ScxmlTag *parent, ScxmlTag *child);
    void addTags(ScxmlTag *parent, const QVector<ScxmlTag*> tags);

    /**
     * @brief removeTag - inform views that tag will be removed
     * @param tag
     */
    void removeTag(ScxmlTag *tag);

    /**
     * @brief changeParent - inform views that tag parent will be changed
     * @param child
     * @param newParent
     * @param tagIndex
     */
    void changeParent(ScxmlTag *child, ScxmlTag *newParent, int tagIndex = -1);

    /**
     * @brief changeOrder - inform vies that tag's child-position will be changed
     * @param child
     * @param newPos
     */
    void changeOrder(ScxmlTag *child, int newPos);

    /**
     * @brief changed - holds the changes-status of the document
     * @return - true if changed, false otherwise
     */
    bool changed() const;

    /**
     * @brief rootTag - return rootTag of the document
     * @return
     */
    ScxmlTag *rootTag() const;
    ScxmlTag *scxmlRootTag() const;
    void pushRootTag(ScxmlTag *tag);
    ScxmlTag *popRootTag();

    /**
     * @brief fileName - return current filename of the document
     * @return
     */
    QString fileName() const;
    void setFileName(const QString &filename);

    /**
     * @brief undoStack - return undo-stack of the document (see UndoStack)
     * @return
     */
    QUndoStack *undoStack() const;

    /**
     * @brief save - save document with given filename
     * @param fileName -
     * @return return true if file has successfully wrote
     */
    bool save(const QString &fileName);
    bool save();
    void load(const QString &fileName);
    bool pasteData(const QByteArray &data, const QPointF &minPos = QPointF(0, 0), const QPointF &pastePos = QPointF(0, 0));
    bool load(QIODevice *io);

    /**
     * @brief nextId - generate and return next id depends on the given key
     * @param key
     * @return return value will be the form key_nextId
     */
    QString nextUniqueId(const QString &key);
    QString getUniqueCopyId(const ScxmlTag *tag);

    /**
     * @brief hasLayouted - tells is there qt-namespace available in the current loaded document
     * @return true or false
     */
    bool hasLayouted() const;

    void setLevelColors(const QVector<QColor> &colors);
    QColor getColor(int depth) const;

    bool hasError() const
    {
        return m_hasError;
    }

    QString lastError() const
    {
        return m_lastError;
    }

    QByteArray content(const QVector<ScxmlTag*> &tags) const;
    QByteArray content(ScxmlTag *tag = nullptr) const;
    void clear(bool createRoot = true);

    // Full NameSpace functions
    QString nameSpaceDelimiter() const;
    bool useFullNameSpace() const;
    void setNameSpaceDelimiter(const QString &delimiter);
    void setUseFullNameSpace(bool use);

    void setUndoRedoRunning(bool para);

    QFileInfo qtBinDir() const;

signals:
    /**
     * @brief documentChanged - this signal will be emitted just after document has changed and is not in clean state
     * @param changed
     */
    void documentChanged(bool changed);

    /**
     * @brief beginTagChange - this signal will be emitted just before some tagchange will be made (see TagChange)
     * @param change
     * @param tag
     * @param value
     */
    void beginTagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value);

    /**
     * @brief beginTagChange - this signal will be emitted just after some tagchange has made (see TagChange)
     * @param change
     * @param tag
     * @param value
     */
    void endTagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value);

    /**
     * @brief colorThemeChanged - this signal will be emitted just after color theme has changed
     */
    void colorThemeChanged();

private:
    friend class ScxmlTag;
    friend class ChangeFullNameSpaceCommand;

    void initErrorMessage(const QXmlStreamReader &xml, QIODevice *io);
    bool generateSCXML(QIODevice *io, ScxmlTag *tag = nullptr) const;
    void addChild(ScxmlTag *tag);
    void removeChild(ScxmlTag *tag);
    void printSCXML();
    void deleteRootTags();
    void clearNamespaces();
    void initVariables();
    void addTagRecursive(ScxmlTag *parent, ScxmlTag *tag);
    void removeTagRecursive(ScxmlTag *tag);

    ScxmlTag *createScxmlTag();
    QString m_fileName;
    QUndoStack *m_undoStack;
    QVector<ScxmlTag*> m_tags;
    QHash<QString, int> m_nextIdHash;
    QHash<QString, QString> m_idMap;
    bool m_hasError = false;
    QString m_lastError;
    QVector<ScxmlTag*> m_rootTags;
    QMap<QString, ScxmlNamespace*> m_namespaces;
    QVector<QColor> m_colors;
    bool m_hasLayouted = false;
    QString m_idDelimiter;
    bool m_useFullNameSpace = false;
    ScxmlTag *m_currentTag = nullptr;
    bool m_undoRedoRunning = false;
    QFileInfo m_qtBinDir;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
