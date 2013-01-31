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

#include "resourcefile_p.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>
#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMimeData>
#include <QtAlgorithms>
#include <QTextStream>

#include <QIcon>
#include <QImageReader>

#include <QDomDocument>

using namespace ResourceEditor;
using namespace ResourceEditor::Internal;

/******************************************************************************
** File
*/

File::File(Prefix *prefix, const QString &_name, const QString &_alias)
    : Node(this, prefix)
    , name(_name)
    , alias(_alias)
    , m_checked(false)
    , m_exists(false)
{
}

void File::checkExistence()
{
    m_checked = false;
}

bool File::exists()
{
    if (!m_checked) {
        m_exists = QFile::exists(name);
        m_checked = true;
    }

    return m_exists;
}

void File::setExists(bool exists)
{
    m_exists = exists;
}

/******************************************************************************
** FileList
*/

bool FileList::containsFile(File *file)
{
    foreach (const File *tmpFile, *this)
        if (tmpFile->name == file->name && tmpFile->prefix() == file->prefix())
            return true;
    return false;
}

/******************************************************************************
** ResourceFile
*/

ResourceFile::ResourceFile(const QString &file_name)
{
    setFileName(file_name);
}

ResourceFile::~ResourceFile()
{
    clearPrefixList();
}

bool ResourceFile::load()
{
    m_error_message.clear();

    if (m_file_name.isEmpty()) {
        m_error_message = tr("The file name is empty.");
        return false;
    }

    QFile file(m_file_name);
    if (!file.open(QIODevice::ReadOnly)) {
        m_error_message = file.errorString();
        return false;
    }

    clearPrefixList();

    QDomDocument doc;

    QString error_msg;
    int error_line, error_col;
    if (!doc.setContent(&file, &error_msg, &error_line, &error_col)) {
        m_error_message = tr("XML error on line %1, col %2: %3")
                    .arg(error_line).arg(error_col).arg(error_msg);
        return false;
    }

    QDomElement root = doc.firstChildElement(QLatin1String("RCC"));
    if (root.isNull()) {
        m_error_message = tr("The <RCC> root element is missing.");
        return false;
    }

    QDomElement relt = root.firstChildElement(QLatin1String("qresource"));
    for (; !relt.isNull(); relt = relt.nextSiblingElement(QLatin1String("qresource"))) {

        QString prefix = fixPrefix(relt.attribute(QLatin1String("prefix")));
        if (prefix.isEmpty())
            prefix = QString(QLatin1Char('/'));
        const QString language = relt.attribute(QLatin1String("lang"));

        const int idx = indexOfPrefix(prefix);
        Prefix * p = 0;
        if (idx == -1) {
            p = new Prefix(prefix, language);
            m_prefix_list.append(p);
        } else {
            p = m_prefix_list[idx];
        }
        Q_ASSERT(p);

        QDomElement felt = relt.firstChildElement(QLatin1String("file"));
        for (; !felt.isNull(); felt = felt.nextSiblingElement(QLatin1String("file"))) {
            const QString fileName = absolutePath(felt.text());
            const QString alias = felt.attribute(QLatin1String("alias"));
            File * const file = new File(p, fileName, alias);
            p->file_list.append(file);
        }
    }

    return true;
}

bool ResourceFile::save()
{
    m_error_message.clear();

    if (m_file_name.isEmpty()) {
        m_error_message = tr("The file name is empty.");
        return false;
    }

    QFile file(m_file_name);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error_message = file.errorString();
        return false;
    }

    QDomDocument doc;
    QDomElement root = doc.createElement(QLatin1String("RCC"));
    doc.appendChild(root);

    const QStringList name_list = prefixList();

    foreach (const QString &name, name_list) {
        FileList file_list;
        QString lang;
        foreach (const Prefix *pref, m_prefix_list) {
            if (pref->name == name){
                file_list += pref->file_list;
                lang = pref->lang;
            }
        }

        QDomElement relt = doc.createElement(QLatin1String("qresource"));
        root.appendChild(relt);
        relt.setAttribute(QLatin1String("prefix"), name);
        if (!lang.isEmpty())
            relt.setAttribute(QLatin1String("lang"), lang);

        foreach (const File *f, file_list) {
            const File &file = *f;
            QDomElement felt = doc.createElement(QLatin1String("file"));
            relt.appendChild(felt);
            const QString conv_file = relativePath(file.name).replace(QDir::separator(), QLatin1Char('/'));
            const QDomText text = doc.createTextNode(conv_file);
            felt.appendChild(text);
            if (!file.alias.isEmpty())
                felt.setAttribute(QLatin1String("alias"), file.alias);
        }
    }

    QTextStream stream(&file);
    doc.save(stream, 4);
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        m_error_message = tr("Cannot write file. Disk full?");
        return false;
    }
#endif
    return true;
}

void ResourceFile::refresh()
{
    for (int i = 0; i < prefixCount(); ++i) {
        const FileList &file_list = m_prefix_list.at(i)->file_list;
        foreach (File *file, file_list)
            file->checkExistence();
    }
}

bool ResourceFile::split(const QString &_path, QString *prefix, QString *file) const
{
    prefix->clear();
    file->clear();

    QString path = _path;
    if (!path.startsWith(QLatin1Char(':')))
        return false;
    path = path.mid(1);

    for (int i = 0; i < m_prefix_list.size(); ++i) {
        Prefix const * const &pref = m_prefix_list.at(i);
        if (!path.startsWith(pref->name))
            continue;

        *prefix = pref->name;
        if (pref->name == QString(QLatin1Char('/')))
            *file = path.mid(1);
        else
            *file = path.mid(pref->name.size() + 1);

        const QString filePath = absolutePath(*file);

        for (int j = 0; j < pref->file_list.count(); j++) {
            File const * const &f = pref->file_list.at(j);
            if (!f->alias.isEmpty()) {
                if (absolutePath(f->alias) == filePath) {
                    *file = f->name;
                    return true;
                }
            } else if (f->name == filePath)
                return true;
        }
    }

    return false;
}

QString ResourceFile::resolvePath(const QString &path) const
{
    QString prefix, file;
    if (split(path, &prefix, &file))
        return absolutePath(file);

    return QString();
}

QStringList ResourceFile::prefixList() const
{
    QStringList result;
    for (int i = 0; i < m_prefix_list.size(); ++i)
        result.append(m_prefix_list.at(i)->name);
    return result;
}

bool ResourceFile::isEmpty() const
{
    return m_file_name.isEmpty() && m_prefix_list.isEmpty();
}

QStringList ResourceFile::fileList(int pref_idx) const
{
    QStringList result;
    Q_ASSERT(pref_idx >= 0 && pref_idx < m_prefix_list.count());
    const FileList &abs_file_list = m_prefix_list.at(pref_idx)->file_list;
    foreach (const File *abs_file, abs_file_list)
        result.append(relativePath(abs_file->name));
    return result;
}

void ResourceFile::addFile(int prefix_idx, const QString &file, int file_idx)
{
    Prefix * const p = m_prefix_list[prefix_idx];
    Q_ASSERT(p);
    FileList &files = p->file_list;
    Q_ASSERT(file_idx >= -1 && file_idx <= files.size());
    if (file_idx == -1)
        file_idx = files.size();
    files.insert(file_idx, new File(p, absolutePath(file)));
}

void ResourceFile::addPrefix(const QString &prefix, int prefix_idx)
{
    QString fixed_prefix = fixPrefix(prefix);
    if (indexOfPrefix(fixed_prefix) != -1)
        return;

    Q_ASSERT(prefix_idx >= -1 && prefix_idx <= m_prefix_list.size());
    if (prefix_idx == -1)
        prefix_idx = m_prefix_list.size();
    m_prefix_list.insert(prefix_idx, new Prefix(fixed_prefix));
}

void ResourceFile::removePrefix(int prefix_idx)
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    Prefix * const p = m_prefix_list.at(prefix_idx);
    delete p;
    m_prefix_list.removeAt(prefix_idx);
}

void ResourceFile::removeFile(int prefix_idx, int file_idx)
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    FileList &fileList = m_prefix_list[prefix_idx]->file_list;
    Q_ASSERT(file_idx >= 0 && file_idx < fileList.count());
    delete fileList.at(file_idx);
    fileList.removeAt(file_idx);
}

void ResourceFile::replacePrefix(int prefix_idx, const QString &prefix)
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    m_prefix_list[prefix_idx]->name = fixPrefix(prefix);
}

void ResourceFile::replaceLang(int prefix_idx, const QString &lang)
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    m_prefix_list[prefix_idx]->lang = lang;
}

void ResourceFile::replaceAlias(int prefix_idx, int file_idx, const QString &alias)
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    FileList &fileList = m_prefix_list.at(prefix_idx)->file_list;
    Q_ASSERT(file_idx >= 0 && file_idx < fileList.count());
    fileList[file_idx]->alias = alias;
}

bool ResourceFile::renameFile(const QString fileName, const QString &newFileName)
{
    bool success = true;

    FileList entries;
    for (int i = 0; i < prefixCount(); ++i) {
        const FileList &file_list = m_prefix_list.at(i)->file_list;
        foreach (File *file, file_list) {
            if (file->name == fileName)
                entries.append(file);
            if (file->name == newFileName)
                return false; // prevent conflicts
        }
    }

    Q_ASSERT(!entries.isEmpty());

    entries.at(0)->checkExistence();
    if (entries.at(0)->exists()) {
        foreach (File *file, entries)
            file->setExists(true);
        success = Core::FileUtils::renameFile(entries.at(0)->name, newFileName);
    }

    if (success) {
        bool exists = QFile::exists(newFileName);
        foreach (File *file, entries) {
            file->name = newFileName;
            file->setExists(exists);
        }
    }

    return success;
}


void ResourceFile::replaceFile(int pref_idx, int file_idx, const QString &file)
{
    Q_ASSERT(pref_idx >= 0 && pref_idx < m_prefix_list.count());
    FileList &fileList = m_prefix_list.at(pref_idx)->file_list;
    Q_ASSERT(file_idx >= 0 && file_idx < fileList.count());
    fileList[file_idx]->name = file;
}

int ResourceFile::indexOfPrefix(const QString &prefix) const
{
    QString fixed_prefix = fixPrefix(prefix);
    for (int i = 0; i < m_prefix_list.size(); ++i) {
        if (m_prefix_list.at(i)->name == fixed_prefix)
            return i;
    }
    return -1;
}

int ResourceFile::indexOfFile(int pref_idx, const QString &file) const
{
    Q_ASSERT(pref_idx >= 0 && pref_idx < m_prefix_list.count());
    Prefix * const p = m_prefix_list.at(pref_idx);
    File equalFile(p, absolutePath(file));
    return p->file_list.indexOf(&equalFile);
}

QString ResourceFile::relativePath(const QString &abs_path) const
{
    if (m_file_name.isEmpty() || QFileInfo(abs_path).isRelative())
         return abs_path;

    QFileInfo fileInfo(m_file_name);
    return fileInfo.absoluteDir().relativeFilePath(abs_path);
}

QString ResourceFile::absolutePath(const QString &rel_path) const
{
    const QFileInfo fi(rel_path);
    if (fi.isAbsolute())
        return rel_path;

    QString rc = QFileInfo(m_file_name).path();
    rc +=  QDir::separator();
    rc += rel_path;
    return QDir::cleanPath(rc);
}

bool ResourceFile::contains(const QString &prefix, const QString &file) const
{
    int pref_idx = indexOfPrefix(prefix);
    if (pref_idx == -1)
        return false;
    if (file.isEmpty())
        return true;
    Q_ASSERT(pref_idx >= 0 && pref_idx < m_prefix_list.count());
    Prefix * const p = m_prefix_list.at(pref_idx);
    Q_ASSERT(p);
    File equalFile(p, absolutePath(file));
    return p->file_list.containsFile(&equalFile);
}

bool ResourceFile::contains(int pref_idx, const QString &file) const
{
    Q_ASSERT(pref_idx >= 0 && pref_idx < m_prefix_list.count());
    Prefix * const p = m_prefix_list.at(pref_idx);
    File equalFile(p, absolutePath(file));
    return p->file_list.containsFile(&equalFile);
}

/*static*/ QString ResourceFile::fixPrefix(const QString &prefix)
{
    const QChar slash = QLatin1Char('/');
    QString result = QString(slash);
    for (int i = 0; i < prefix.size(); ++i) {
        const QChar c = prefix.at(i);
        if (c == slash && result.at(result.size() - 1) == slash)
            continue;
        result.append(c);
    }

    if (result.size() > 1 && result.endsWith(slash))
        result = result.mid(0, result.size() - 1);

    return result;
}

int ResourceFile::prefixCount() const
{
    return m_prefix_list.size();
}

QString ResourceFile::prefix(int idx) const
{
    Q_ASSERT((idx >= 0) && (idx < m_prefix_list.count()));
    return m_prefix_list.at(idx)->name;
}

QString ResourceFile::lang(int idx) const
{
    Q_ASSERT(idx >= 0 && idx < m_prefix_list.count());
    return m_prefix_list.at(idx)->lang;
}

int ResourceFile::fileCount(int prefix_idx) const
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    return m_prefix_list.at(prefix_idx)->file_list.size();
}

QString ResourceFile::file(int prefix_idx, int file_idx) const
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    FileList &fileList = m_prefix_list.at(prefix_idx)->file_list;
    Q_ASSERT(file_idx >= 0 && file_idx < fileList.count());
    fileList.at(file_idx)->checkExistence();
    return fileList.at(file_idx)->name;
}

QString ResourceFile::alias(int prefix_idx, int file_idx) const
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    FileList &fileList = m_prefix_list.at(prefix_idx)->file_list;
    Q_ASSERT(file_idx >= 0 && file_idx < fileList.count());
    return fileList.at(file_idx)->alias;
}

void * ResourceFile::prefixPointer(int prefixIndex) const
{
    Q_ASSERT(prefixIndex >= 0 && prefixIndex < m_prefix_list.count());
    return m_prefix_list.at(prefixIndex);
}

void * ResourceFile::filePointer(int prefixIndex, int fileIndex) const
{
    Q_ASSERT(prefixIndex >= 0 && prefixIndex < m_prefix_list.count());
    FileList &fileList = m_prefix_list.at(prefixIndex)->file_list;
    Q_ASSERT(fileIndex >= 0 && fileIndex < fileList.count());
    return fileList.at(fileIndex);
}

int ResourceFile::prefixPointerIndex(const Prefix *prefix) const
{
    int const count = m_prefix_list.count();
    for (int i = 0; i < count; i++) {
        Prefix * const other = m_prefix_list.at(i);
        if (*other == *prefix)
            return i;
    }
    return -1;
}

void ResourceFile::clearPrefixList()
{
    qDeleteAll(m_prefix_list);
    m_prefix_list.clear();
}

/******************************************************************************
** ResourceModel
*/

ResourceModel::ResourceModel(const ResourceFile &resource_file, QObject *parent)
    : QAbstractItemModel(parent), m_resource_file(resource_file),  m_dirty(false)
{
    // Only action that works for QListWidget and the like.
    setSupportedDragActions(Qt::CopyAction);

    m_prefixIcon = Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon,
        QIcon(QLatin1String(":/resourceeditor/images/qt_qrc.png")), QSize(16, 16));
}

void ResourceModel::setDirty(bool b)
{
    if (b == m_dirty)
        return;

    m_dirty = b;
    emit dirtyChanged(b);
}

QModelIndex ResourceModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0)
        return QModelIndex();

    void * internalPointer = 0;
    if (parent.isValid()) {
        void * const pip = parent.internalPointer();
        if (pip == 0)
            return QModelIndex();

        // File node
        Node * const node = reinterpret_cast<Node *>(pip);
        Prefix * const prefix = node->prefix();
        Q_ASSERT(prefix);
        if (row < 0 || row >= prefix->file_list.count())
            return QModelIndex();
        const int prefixIndex = m_resource_file.prefixPointerIndex(prefix);
        const int fileIndex = row;
        internalPointer = m_resource_file.filePointer(prefixIndex, fileIndex);
    } else {
        // Prefix node
        if (row < 0 || row >= m_resource_file.prefixCount())
            return QModelIndex();
        internalPointer = m_resource_file.prefixPointer(row);
    }
    Q_ASSERT(internalPointer);
    return createIndex(row, 0, internalPointer);
}

QModelIndex ResourceModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    void * const internalPointer = index.internalPointer();
    if (internalPointer == 0)
        return QModelIndex();
    Node * const node = reinterpret_cast<Node *>(internalPointer);
    Prefix * const prefix = node->prefix();
    Q_ASSERT(prefix);
    bool const isFileNode = (prefix != node);

    if (isFileNode) {
        const int row = m_resource_file.prefixPointerIndex(prefix);
        Q_ASSERT(row >= 0);
        return createIndex(row, 0, prefix);
    } else {
        return QModelIndex();
    }
}

int ResourceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        void * const internalPointer = parent.internalPointer();
        Node * const node = reinterpret_cast<Node *>(internalPointer);
        Prefix * const prefix = node->prefix();
        Q_ASSERT(prefix);
        bool const isFileNode = (prefix != node);

        if (isFileNode)
            return 0;
        else
            return prefix->file_list.count();
    } else {
        return m_resource_file.prefixCount();
    }
}

int ResourceModel::columnCount(const QModelIndex &) const
{
    return 1;
}

bool ResourceModel::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent) != 0;
}

void ResourceModel::refresh()
{
    m_resource_file.refresh();
}

Qt::ItemFlags ResourceModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(index);

    const void *internalPointer = index.internalPointer();
    const Node *node = reinterpret_cast<const Node *>(internalPointer);
    const Prefix *prefix = node->prefix();
    Q_ASSERT(prefix);
    const bool isFileNode = (prefix != node);

    if (isFileNode)
        f |= Qt::ItemIsEditable;

    return f;
}

bool ResourceModel::iconFileExtension(const QString &path)
{
    static QStringList ext_list;
    if (ext_list.isEmpty()) {
        const QList<QByteArray> _ext_list = QImageReader::supportedImageFormats();
        foreach (const QByteArray &ext, _ext_list) {
            QString dotExt = QString(QLatin1Char('.'));
            dotExt  += QString::fromLatin1(ext);
            ext_list.append(dotExt);
        }
    }

    foreach (const QString &ext, ext_list) {
        if (path.endsWith(ext, Qt::CaseInsensitive))
            return true;
    }

    return false;
}

static inline void appendParenthesized(const QString &what, QString &s)
{
    s += QLatin1String(" (");
    s += what;
    s += QLatin1Char(')');
}

QVariant ResourceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const void *internalPointer = index.internalPointer();
    const Node *node = reinterpret_cast<const Node *>(internalPointer);
    const Prefix *prefix = node->prefix();
    File *file = node->file();
    Q_ASSERT(prefix);
    const bool isFileNode = (prefix != node);

    QVariant result;

    switch (role) {
    case Qt::DisplayRole:
        {
            QString stringRes;
            if (!isFileNode) {
                // Prefix node
                stringRes = prefix->name;
                const QString &lang = prefix->lang;
                if (!lang.isEmpty())
                    appendParenthesized(lang, stringRes);
            } else  {
                // File node
                Q_ASSERT(file);
                QString conv_file = m_resource_file.relativePath(file->name);
                stringRes = QDir::fromNativeSeparators(conv_file);
                const QString alias = file->alias;
                if (!alias.isEmpty())
                    appendParenthesized(alias, stringRes);
            }
            result = stringRes;
        }
        break;
    case Qt::DecorationRole:
        if (isFileNode) {
            // File node
            Q_ASSERT(file);
            if (file->icon.isNull()) {
                const QString path = m_resource_file.absolutePath(file->name);
                if (iconFileExtension(path))
                    file->icon = QIcon(path);
                else
                    file->icon = Core::FileIconProvider::instance()->icon(QFileInfo(path));
            }
            if (!file->icon.isNull())
                result = file->icon;

        } else {
            result = m_prefixIcon;
        }
        break;
    case Qt::EditRole:
        if (isFileNode) {
            Q_ASSERT(file);
            QString conv_file = m_resource_file.relativePath(file->name);
            result = QDir::fromNativeSeparators(conv_file);
        }
        break;
    case Qt::ForegroundRole:
        if (isFileNode) {
            // File node
            Q_ASSERT(file);
            if (!file->exists())
                result = QBrush(QColor(Qt::red));
        }
        break;
    default:
        break;
    }
    return result;
}

bool ResourceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role != Qt::EditRole)
        return false;

    const QDir baseDir = QFileInfo(fileName()).absoluteDir();
    Utils::FileName newFileName = Utils::FileName::fromUserInput(
                baseDir.absoluteFilePath(value.toString()));

    if (newFileName.isEmpty())
        return false;

    if (!newFileName.isChildOf(baseDir))
        return false;

    return renameFile(file(index), newFileName.toString());
}

void ResourceModel::getItem(const QModelIndex &index, QString &prefix, QString &file) const
{
    prefix.clear();
    file.clear();

    if (!index.isValid())
        return;

    const void *internalPointer = index.internalPointer();
    const Node *node = reinterpret_cast<const Node *>(internalPointer);
    const Prefix *p = node->prefix();
    Q_ASSERT(p);
    const bool isFileNode = (p != node);

    if (isFileNode) {
        const File *f = node->file();
        Q_ASSERT(f);
        if (!f->alias.isEmpty())
            file = f->alias;
        else
            file = f->name;
    } else {
        prefix = p->name;
    }
}

QString ResourceModel::lang(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();

    return m_resource_file.lang(index.row());
}

QString ResourceModel::alias(const QModelIndex &index) const
{
    if (!index.isValid() || !index.parent().isValid())
        return QString();
    return m_resource_file.alias(index.parent().row(), index.row());
}

QString ResourceModel::file(const QModelIndex &index) const
{
    if (!index.isValid() || !index.parent().isValid())
        return QString();
    return m_resource_file.file(index.parent().row(), index.row());
}

QModelIndex ResourceModel::getIndex(const QString &prefixed_file)
{
    QString prefix, file;
    if (!m_resource_file.split(prefixed_file, &prefix, &file))
        return QModelIndex();
    return getIndex(prefix, file);
}

QModelIndex ResourceModel::getIndex(const QString &prefix, const QString &file)
{
    if (prefix.isEmpty())
        return QModelIndex();

    const int pref_idx = m_resource_file.indexOfPrefix(prefix);
    if (pref_idx == -1)
        return QModelIndex();

    const QModelIndex pref_model_idx = index(pref_idx, 0, QModelIndex());
    if (file.isEmpty())
        return pref_model_idx;

    const int file_idx = m_resource_file.indexOfFile(pref_idx, file);
    if (file_idx == -1)
        return QModelIndex();

    return index(file_idx, 0, pref_model_idx);
}

QModelIndex ResourceModel::prefixIndex(const QModelIndex &sel_idx) const
{
    if (!sel_idx.isValid())
        return QModelIndex();
    const QModelIndex parentIndex = parent(sel_idx);
    return parentIndex.isValid() ? parentIndex : sel_idx;
}

QModelIndex ResourceModel::addNewPrefix()
{
    const QString format = QLatin1String("/new/prefix%1");
    int i = 1;
    QString prefix = format.arg(i);
    for ( ; m_resource_file.contains(prefix); i++)
        prefix = format.arg(i);

    i = rowCount(QModelIndex());
    beginInsertRows(QModelIndex(), i, i);
    m_resource_file.addPrefix(prefix);
    endInsertRows();

    setDirty(true);

    return index(i, 0, QModelIndex());
}

QModelIndex ResourceModel::addFiles(const QModelIndex &model_idx, const QStringList &file_list)
{
    const QModelIndex prefixModelIndex = prefixIndex(model_idx);
    const int prefixArrayIndex = prefixModelIndex.row();
    const int cursorFileArrayIndex = (prefixModelIndex == model_idx) ? 0 : model_idx.row();
    int dummy;
    int lastFileArrayIndex;
    addFiles(prefixArrayIndex, file_list, cursorFileArrayIndex, dummy, lastFileArrayIndex);
    return index(lastFileArrayIndex, 0, prefixModelIndex);
}

QStringList ResourceModel::existingFilesSubtracted(int prefixIndex, const QStringList &fileNames) const
{
    const QModelIndex prefixModelIdx = index(prefixIndex, 0, QModelIndex());
    QStringList uniqueList;

    if (prefixModelIdx.isValid()) {
        foreach (const QString &file, fileNames) {
            if (!m_resource_file.contains(prefixIndex, file) && !uniqueList.contains(file))
                uniqueList.append(file);
        }
    }
    return uniqueList;
}

void ResourceModel::addFiles(int prefixIndex, const QStringList &fileNames, int cursorFile,
        int &firstFile, int &lastFile)
{
    Q_UNUSED(cursorFile)
    const QModelIndex prefix_model_idx = index(prefixIndex, 0, QModelIndex());
    firstFile = -1;
    lastFile = -1;

    if (!prefix_model_idx.isValid())
        return;

    QStringList unique_list = existingFilesSubtracted(prefixIndex, fileNames);

    if (unique_list.isEmpty())
        return;

    const int cnt = m_resource_file.fileCount(prefixIndex);
    beginInsertRows(prefix_model_idx, cnt, cnt + unique_list.count() - 1); // ### FIXME

    foreach (const QString &file, unique_list)
        m_resource_file.addFile(prefixIndex, file);

    const QFileInfo fi(unique_list.last());
    m_lastResourceDir = fi.absolutePath();

    endInsertRows();
    setDirty(true);

    firstFile = cnt;
    lastFile = cnt + unique_list.count() - 1;

    Core::ICore::vcsManager()->promptToAdd(QFileInfo(m_resource_file.fileName()).absolutePath(),
                                           fileNames);
}


void ResourceModel::insertPrefix(int prefixIndex, const QString &prefix,
        const QString &lang)
{
    beginInsertRows(QModelIndex(), prefixIndex, prefixIndex);
    m_resource_file.addPrefix(prefix, prefixIndex);
    m_resource_file.replaceLang(prefixIndex, lang);
    endInsertRows();
    setDirty(true);
}

void ResourceModel::insertFile(int prefixIndex, int fileIndex,
        const QString &fileName, const QString &alias)
{
    const QModelIndex parent = index(prefixIndex, 0, QModelIndex());
    beginInsertRows(parent, fileIndex, fileIndex);
    m_resource_file.addFile(prefixIndex, fileName, fileIndex);
    m_resource_file.replaceAlias(prefixIndex, fileIndex, alias);
    endInsertRows();
    setDirty(true);
}

bool ResourceModel::renameFile(const QString &fileName, const QString &newFileName)
{
    bool success = m_resource_file.renameFile(fileName, newFileName);
    if (success)
        setDirty(true);
    return success;
}

void ResourceModel::changePrefix(const QModelIndex &model_idx, const QString &prefix)
{
    if (!model_idx.isValid())
        return;

    const QModelIndex prefix_model_idx = prefixIndex(model_idx);
    const int prefix_idx = model_idx.row();
    if (m_resource_file.prefix(prefix_idx) == ResourceFile::fixPrefix(prefix))
        return;

    if (m_resource_file.contains(prefix))
        return;

    m_resource_file.replacePrefix(prefix_idx, prefix);
    emit dataChanged(prefix_model_idx, prefix_model_idx);
    setDirty(true);
}

void ResourceModel::changeLang(const QModelIndex &model_idx, const QString &lang)
{
    if (!model_idx.isValid())
        return;

    const QModelIndex prefix_model_idx = prefixIndex(model_idx);
    const int prefix_idx = model_idx.row();
    if (m_resource_file.lang(prefix_idx) == lang)
        return;

    m_resource_file.replaceLang(prefix_idx, lang);
    emit dataChanged(prefix_model_idx, prefix_model_idx);
    setDirty(true);
}

void ResourceModel::changeAlias(const QModelIndex &index, const QString &alias)
{
    if (!index.parent().isValid())
        return;

    if (m_resource_file.alias(index.parent().row(), index.row()) == alias)
        return;
    m_resource_file.replaceAlias(index.parent().row(), index.row(), alias);
    emit dataChanged(index, index);
    setDirty(true);
}

QModelIndex ResourceModel::deleteItem(const QModelIndex &idx)
{
    if (!idx.isValid())
        return QModelIndex();

    QString dummy, file;
    getItem(idx, dummy, file);
    int prefix_idx = -1;
    int file_idx = -1;

    beginRemoveRows(parent(idx), idx.row(), idx.row());
    if (file.isEmpty()) {
        // Remove prefix
        prefix_idx = idx.row();
        m_resource_file.removePrefix(prefix_idx);
        if (prefix_idx == m_resource_file.prefixCount())
            --prefix_idx;
    } else {
        // Remove file
        prefix_idx = prefixIndex(idx).row();
        file_idx = idx.row();
        m_resource_file.removeFile(prefix_idx, file_idx);
        if (file_idx == m_resource_file.fileCount(prefix_idx))
            --file_idx;
    }
    endRemoveRows();

    setDirty(true);

    if (prefix_idx == -1)
        return QModelIndex();
    const QModelIndex prefix_model_idx = index(prefix_idx, 0, QModelIndex());
    if (file_idx == -1)
        return prefix_model_idx;
    return index(file_idx, 0, prefix_model_idx);
}

bool ResourceModel::reload()
{
    beginResetModel();
    const bool result = m_resource_file.load();
    if (result)
        setDirty(false);
    endResetModel();
    return result;
}

bool ResourceModel::save()
{
    const bool result = m_resource_file.save();
    if (result)
        setDirty(false);
    return result;
}

QString ResourceModel::lastResourceOpenDirectory() const
{
    if (m_lastResourceDir.isEmpty())
        return absolutePath(QString());
    return m_lastResourceDir;
}

// Create a resource path 'prefix:/file'
QString ResourceModel::resourcePath(const QString &prefix, const QString &file)
{
    QString rc = QString(QLatin1Char(':'));
    rc += prefix;
    rc += QLatin1Char('/');
    rc += file;
    return QDir::cleanPath(rc);
}

QMimeData *ResourceModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.size() != 1)
        return 0;

    QString prefix, file;
    getItem(indexes.front(), prefix, file);
    if (prefix.isEmpty() || file.isEmpty())
        return 0;

    // DnD format of Designer 4.4
    QDomDocument doc;
    QDomElement elem = doc.createElement(QLatin1String("resource"));
    elem.setAttribute(QLatin1String("type"), QLatin1String("image"));
    elem.setAttribute(QLatin1String("file"), resourcePath(prefix, file));
    doc.appendChild(elem);

    QMimeData *rc = new QMimeData;
    rc->setText(doc.toString());
    return rc;
}
