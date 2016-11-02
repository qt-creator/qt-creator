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

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QIcon>

#include <coreplugin/idocument.h>
#include <utils/textfileformat.h>

namespace ResourceEditor {
namespace Internal {

class File;
struct Prefix;

/*!
    \class Node

    Forms the base class for nodes in a \l ResourceFile tree.
*/
class Node
{
protected:
    Node(File *file, Prefix *prefix) : m_file(file), m_prefix(prefix)
    {
        Q_ASSERT(m_prefix);
    }
public:
    File *file() const { return m_file; }
    Prefix *prefix() const { return m_prefix; }
private:
    File *m_file;
    Prefix *m_prefix;
};

/*!
    \class File

    Represents a file node in a \l ResourceFile tree.
*/
class File : public Node
{
public:
    File(Prefix *prefix, const QString &_name, const QString &_alias = QString());
    void checkExistence();
    bool exists();
    void setExists(bool exists);

    bool operator < (const File &other) const { return name < other.name; }
    bool operator == (const File &other) const { return name == other.name; }
    bool operator != (const File &other) const { return name != other.name; }

    QString name;
    QString alias;
    QIcon icon;

    // not used, only loaded and saved
    QString compress;
    QString threshold;

private:
    bool m_checked;
    bool m_exists;
};

class FileList : public QList<File *>
{
public:
    bool containsFile(File *file);
};

/*!
    \class Prefix

    Represents a prefix node in a \l ResourceFile tree.
*/
struct Prefix : public Node
{
    Prefix(const QString &_name = QString(), const QString &_lang = QString(), const FileList &_file_list = FileList())
        : Node(NULL, this), name(_name), lang(_lang), file_list(_file_list) {}
    ~Prefix()
    {
        qDeleteAll(file_list);
        file_list.clear();
    }
    bool operator == (const Prefix &other) const { return (name == other.name) && (lang == other.lang); }
    QString name;
    QString lang;
    FileList file_list;
};
typedef QList<Prefix *> PrefixList;

/*!
    \class ResourceFile

    Represents the structure of a Qt Resource File (.qrc) file.
*/
class ResourceFile
{
    Q_DECLARE_TR_FUNCTIONS(ResourceFile)
public:
    ResourceFile(const QString &file_name = QString(), const QString &contents = QString());
    ~ResourceFile();

    void setFileName(const QString &file_name) { m_file_name = file_name; }
    QString fileName() const { return m_file_name; }
    Core::IDocument::OpenResult load();
    bool save();
    QString contents() const;
    QString errorMessage() const { return m_error_message; }
    void refresh();

    int prefixCount() const;
    QString prefix(int idx) const;
    QString lang(int idx) const;

    int fileCount(int prefix_idx) const;

    QString file(int prefix_idx, int file_idx) const;
    QString alias(int prefix_idx, int file_idx) const;

    int addFile(int prefix_idx, const QString &file, int file_idx = -1);
    int addPrefix(const QString &prefix, const QString &lang, int prefix_idx = -1);

    void removePrefix(int prefix_idx);
    void removeFile(int prefix_idx, int file_idx);

    bool replacePrefix(int prefix_idx, const QString &prefix);
    bool replaceLang(int prefix_idx, const QString &lang);
    bool replacePrefixAndLang(int prefix_idx, const QString &prefix, const QString &lang);
    void replaceAlias(int prefix_idx, int file_idx, const QString &alias);

    bool renameFile(const QString &fileName, const QString &newFileName);

    void replaceFile(int pref_idx, int file_idx, const QString &file);
    int indexOfPrefix(const QString &prefix, const QString &lang) const;
    int indexOfFile(int pref_idx, const QString &file) const;

    bool contains(const QString &prefix, const QString &lang, const QString &file = QString()) const;
    bool contains(int pref_idx, const QString &file) const;

    QString relativePath(const QString &abs_path) const;
    QString absolutePath(const QString &rel_path) const;

    static QString fixPrefix(const QString &prefix);

private:
    PrefixList m_prefix_list;
    QString m_file_name;
    QString m_contents;
    QString m_error_message;
    Utils::TextFileFormat m_textFileFormat;

public:
    void * prefixPointer(int prefixIndex) const;
    void * filePointer(int prefixIndex, int fileIndex) const;
    int prefixPointerIndex(const Prefix *prefix) const;

private:
    void clearPrefixList();
    int indexOfPrefix(const QString &prefix, const QString &lang, int skip) const;
};

/*!
    \class ResourceModel

    Wraps a \l ResourceFile as a single-column tree model.
*/
class ResourceModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ResourceModel(QObject *parent = 0);

    QModelIndex index(int row, int column,
                        const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void refresh();

    QString errorMessage() const;

    QList<QModelIndex> nonExistingFiles() const;

protected:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

public:
    QString fileName() const { return m_resource_file.fileName(); }
    void setFileName(const QString &file_name) { m_resource_file.setFileName(file_name); }
    void getItem(const QModelIndex &index, QString &prefix, QString &file) const;

    QString lang(const QModelIndex &index) const;
    QString alias(const QModelIndex &index) const;
    QString file(const QModelIndex &index) const;

    virtual QModelIndex addNewPrefix();
    virtual QModelIndex addFiles(const QModelIndex &idx, const QStringList &file_list);
    QStringList existingFilesSubtracted(int prefixIndex, const QStringList &fileNames) const;
    void addFiles(int prefixIndex, const QStringList &fileNames, int cursorFile, int &firstFile, int &lastFile);
    void insertPrefix(int prefixIndex, const QString &prefix, const QString &lang);
    void insertFile(int prefixIndex, int fileIndex, const QString &fileName, const QString &alias);
    virtual void changePrefix(const QModelIndex &idx, const QString &prefix);
    virtual void changeLang(const QModelIndex &idx, const QString &lang);
    virtual void changeAlias(const QModelIndex &idx, const QString &alias);
    virtual QModelIndex deleteItem(const QModelIndex &idx);
    QModelIndex getIndex(const QString &prefix, const QString &lang, const QString &file);
    QModelIndex prefixIndex(const QModelIndex &sel_idx) const;

    QString absolutePath(const QString &path) const
        { return m_resource_file.absolutePath(path); }
    QString relativePath(const QString &path) const
        { return m_resource_file.relativePath(path); }

private:
    QString lastResourceOpenDirectory() const;
    bool renameFile(const QString &fileName, const QString &newFileName);

public:
    virtual Core::IDocument::OpenResult reload();
    virtual bool save();
    QString contents() const { return m_resource_file.contents(); }

    // QString errorMessage() const { return m_resource_file.errorMessage(); }

    bool dirty() const { return m_dirty; }
    void setDirty(bool b);

private:
    virtual QMimeData *mimeData (const QModelIndexList & indexes) const;

    static bool iconFileExtension(const QString &path);
    static QString resourcePath(const QString &prefix, const QString &file);

signals:
    void dirtyChanged(bool b);
    void contentsChanged();

private:
    ResourceFile m_resource_file;
    bool m_dirty;
    QString m_lastResourceDir;
    QIcon m_prefixIcon;
};

/*!
    \class EntryBackup

    Holds the backup of a tree node including children.
*/
class EntryBackup
{
protected:
    ResourceModel *m_model;
    int m_prefixIndex;
    QString m_name;

    EntryBackup(ResourceModel &model, int prefixIndex, const QString &name)
            : m_model(&model), m_prefixIndex(prefixIndex), m_name(name) { }

public:
    virtual void restore() const = 0;
    virtual ~EntryBackup() { }
};

class RelativeResourceModel : public ResourceModel
{
public:
    RelativeResourceModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (!index.isValid())
            return QVariant();
/*
        void const * const internalPointer = index.internalPointer();

        if ((role == Qt::DisplayRole) && (internalPointer != NULL))
            return ResourceModel::data(index, Qt::ToolTipRole);
*/
        return ResourceModel::data(index, role);
    }

    void setResourceDragEnabled(bool e) { m_resourceDragEnabled = e; }
    bool resourceDragEnabled() const { return m_resourceDragEnabled; }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    EntryBackup * removeEntry(const QModelIndex &index);

private:
    bool m_resourceDragEnabled;
};

} // namespace Internal
} // namespace ResourceEditor
