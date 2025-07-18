// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourcefile_p.h"

#include "../resourceeditortr.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/vcsmanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/removefiledialog.h>
#include <utils/theme/theme.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QIcon>
#include <QImageReader>
#include <QMimeData>
#include <QTextStream>
#include <QtAlgorithms>

using namespace Utils;

namespace ResourceEditor::Internal {

File::File(Prefix *prefix, const FilePath &_name, const QString &_alias)
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
        m_exists = name.exists();
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

static bool containsFile(const FileList &list, File *file)
{
    for (const File *tmpFile : list)
        if (tmpFile->name == file->name && tmpFile->prefix() == file->prefix())
            return true;
    return false;
}

/******************************************************************************
** ResourceFile
*/

ResourceFile::ResourceFile(const FilePath &filePath, const QString &contents)
{
    m_filePath = filePath;
    m_contents = contents;
}

ResourceFile::~ResourceFile()
{
    clearPrefixList();
}

Result<> ResourceFile::load()
{
    m_error_message.clear();

    if (m_filePath.isEmpty()) {
        m_error_message = Tr::tr("The file name is empty.");
        return ResultError(m_error_message);
    }

    clearPrefixList();

    QDomDocument doc;

    if (m_contents.isEmpty()) {

        // Regular file
        auto readResult = m_filePath.fileContents();
        if (!readResult){
                m_error_message = readResult.error();
                return ResultError(m_error_message);
        }
        QByteArray data = readResult.value();
        // Detect line ending style
        m_textFileFormat.detectFromData(data);
        // we always write UTF-8 when saving
        m_textFileFormat.setEncoding(TextEncoding::Utf8);

        QString error_msg;
        int error_line, error_col;
        if (!doc.setContent(data, &error_msg, &error_line, &error_col)) {
            m_error_message = Tr::tr("XML error on line %1, col %2: %3")
                        .arg(error_line).arg(error_col).arg(error_msg);
            return ResultError(m_error_message);
        }

    } else {

        // Virtual file from qmake evaluator
        QString error_msg;
        int error_line, error_col;
        if (!doc.setContent(m_contents, &error_msg, &error_line, &error_col)) {
            m_error_message = Tr::tr("XML error on line %1, col %2: %3")
                        .arg(error_line).arg(error_col).arg(error_msg);
            return ResultError(m_error_message);
        }

    }

    QDomElement root = doc.firstChildElement(QLatin1String("RCC"));
    if (root.isNull()) {
        m_error_message = Tr::tr("The <RCC> root element is missing.");
        return ResultError(m_error_message);
    }

    QDomElement relt = root.firstChildElement(QLatin1String("qresource"));
    for (; !relt.isNull(); relt = relt.nextSiblingElement(QLatin1String("qresource"))) {

        QString prefix = fixPrefix(relt.attribute(QLatin1String("prefix")));
        if (prefix.isEmpty())
            prefix = QString(QLatin1Char('/'));
        const QString language = relt.attribute(QLatin1String("lang"));

        const int idx = indexOfPrefix(prefix, language);
        Prefix *p = nullptr;
        if (idx == -1) {
            p = new Prefix(prefix, language);
            m_prefix_list.append(p);
        } else {
            p = m_prefix_list[idx];
        }
        Q_ASSERT(p);

        QDomElement felt = relt.firstChildElement(QLatin1String("file"));
        for (; !felt.isNull(); felt = felt.nextSiblingElement(QLatin1String("file"))) {
            const FilePath fileName = absolutePath(felt.text());
            const QString alias = felt.attribute(QLatin1String("alias"));
            File * const file = new File(p, fileName, alias);
            file->compress = felt.attribute(QLatin1String("compress"));
            file->compressAlgo = felt.attribute(QLatin1String("compress-algo"));
            file->threshold = felt.attribute(QLatin1String("threshold"));
            p->file_list.append(file);
        }
    }

    return ResultOk;
}

QString ResourceFile::contents() const
{
    QDomDocument doc;
    QDomElement root = doc.createElement(QLatin1String("RCC"));
    doc.appendChild(root);

    for (const Prefix *pref : m_prefix_list) {
        const FileList file_list = pref->file_list;
        const QString &name = pref->name;
        const QString &lang = pref->lang;

        QDomElement relt = doc.createElement(QLatin1String("qresource"));
        root.appendChild(relt);
        relt.setAttribute(QLatin1String("prefix"), name);
        if (!lang.isEmpty())
            relt.setAttribute(QLatin1String("lang"), lang);

        for (const File *f : file_list) {
            const File &file = *f;
            QDomElement felt = doc.createElement(QLatin1String("file"));
            relt.appendChild(felt);
            const QString conv_file = relativePath(file.name);
            const QDomText text = doc.createTextNode(conv_file);
            felt.appendChild(text);
            if (!file.alias.isEmpty())
                felt.setAttribute(QLatin1String("alias"), file.alias);
            if (!file.compress.isEmpty())
                felt.setAttribute(QLatin1String("compress"), file.compress);
            if (!file.compressAlgo.isEmpty())
                felt.setAttribute(QLatin1String("compress-algo"), file.compressAlgo);
            if (!file.threshold.isEmpty())
                felt.setAttribute(QLatin1String("threshold"), file.threshold);
        }
    }
    return doc.toString(4);
}

bool ResourceFile::save()
{
    m_error_message.clear();

    if (m_filePath.isEmpty()) {
        m_error_message = Tr::tr("The file name is empty.");
        return false;
    }

    const Result<> res = m_textFileFormat.writeFile(m_filePath, contents());
    if (!res)
        m_error_message = res.error();
    return res.has_value();
}

void ResourceFile::refresh()
{
    for (int i = 0; i < prefixCount(); ++i) {
        const FileList &file_list = m_prefix_list.at(i)->file_list;
        for (File *file : file_list)
            file->checkExistence();
    }
}

int ResourceFile::addFile(int prefix_idx, const QString &file, int file_idx)
{
    Prefix * const p = m_prefix_list[prefix_idx];
    FileList &files = p->file_list;
    Q_ASSERT(file_idx >= -1 && file_idx <= files.size());
    if (file_idx == -1)
        file_idx = files.size();
    files.insert(file_idx, new File(p, absolutePath(file)));
    return file_idx;
}

int ResourceFile::addPrefix(const QString &prefix, const QString &lang, int prefix_idx)
{
    QString fixed_prefix = fixPrefix(prefix);
    if (indexOfPrefix(fixed_prefix, lang) != -1)
        return -1;

    Q_ASSERT(prefix_idx >= -1 && prefix_idx <= m_prefix_list.size());
    if (prefix_idx == -1)
        prefix_idx = m_prefix_list.size();
    m_prefix_list.insert(prefix_idx, new Prefix(fixed_prefix));
    m_prefix_list[prefix_idx]->lang = lang;
    return prefix_idx;
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

bool ResourceFile::replacePrefix(int prefix_idx, const QString &prefix)
{
    const QString fixed_prefix = fixPrefix(prefix);
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    const int existingIndex = indexOfPrefix(fixed_prefix, m_prefix_list.at(prefix_idx)->lang, prefix_idx);
    if (existingIndex != -1) // prevent duplicated prefix + lang combinations
        return false;

    // no change
    if (m_prefix_list.at(prefix_idx)->name == fixed_prefix)
        return false;

    m_prefix_list[prefix_idx]->name = fixed_prefix;
    return true;
}

bool ResourceFile::replaceLang(int prefix_idx, const QString &lang)
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    const int existingIndex = indexOfPrefix(m_prefix_list.at(prefix_idx)->name, lang, prefix_idx);
    if (existingIndex != -1) // prevent duplicated prefix + lang combinations
        return false;

    // no change
    if (m_prefix_list.at(prefix_idx)->lang == lang)
        return false;

    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    m_prefix_list[prefix_idx]->lang = lang;
    return true;
}

bool ResourceFile::replacePrefixAndLang(int prefix_idx, const QString &prefix, const QString &lang)
{
    const QString fixed_prefix = fixPrefix(prefix);
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    const int existingIndex = indexOfPrefix(fixed_prefix, lang, prefix_idx);
    if (existingIndex != -1) // prevent duplicated prefix + lang combinations
        return false;

    // no change
    if (m_prefix_list.at(prefix_idx)->name == fixed_prefix
            && m_prefix_list.at(prefix_idx)->lang == lang)
        return false;

    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    m_prefix_list[prefix_idx]->name = fixed_prefix;
    m_prefix_list[prefix_idx]->lang = lang;
    return true;
}

void ResourceFile::replaceAlias(int prefix_idx, int file_idx, const QString &alias)
{
    Q_ASSERT(prefix_idx >= 0 && prefix_idx < m_prefix_list.count());
    FileList &fileList = m_prefix_list.at(prefix_idx)->file_list;
    Q_ASSERT(file_idx >= 0 && file_idx < fileList.count());
    fileList[file_idx]->alias = alias;
}

bool ResourceFile::renameFile(const FilePath &fileName, const FilePath &newFileName)
{
    QTC_CHECK(fileName.isSameDevice(m_filePath));
    QTC_CHECK(newFileName.isSameDevice(m_filePath));

    bool success = true;

    FileList entries;
    for (int i = 0; i < prefixCount(); ++i) {
        const FileList &file_list = m_prefix_list.at(i)->file_list;
        for (File *file : file_list) {
            if (file->name == fileName)
                entries.append(file);
            if (file->name == newFileName)
                return false; // prevent conflicts
        }
    }

    Q_ASSERT(!entries.isEmpty());

    entries.at(0)->checkExistence();
    if (entries.at(0)->exists()) {
        for (File *file : std::as_const(entries))
            file->setExists(true);
        success = Core::FileUtils::renameFile(entries.at(0)->name, newFileName);
    }

    if (success) {
        const bool exists = newFileName.exists();
        for (File *file : std::as_const(entries)) {
            file->name = newFileName;
            file->setExists(exists);
        }
    }

    return success;
}

void ResourceFile::replaceFile(int pref_idx, int file_idx, const FilePath &file)
{
    Q_ASSERT(pref_idx >= 0 && pref_idx < m_prefix_list.count());
    FileList &fileList = m_prefix_list.at(pref_idx)->file_list;
    Q_ASSERT(file_idx >= 0 && file_idx < fileList.count());
    fileList[file_idx]->name = file;
}

int ResourceFile::indexOfPrefix(const QString &prefix, const QString &lang) const
{
    return indexOfPrefix(prefix, lang, -1);
}

int ResourceFile::indexOfPrefix(const QString &prefix, const QString &lang, int skip) const
{
    QString fixed_prefix = fixPrefix(prefix);
    for (int i = 0; i < m_prefix_list.size(); ++i) {
        if (i == skip)
            continue;
        if (m_prefix_list.at(i)->name == fixed_prefix
                && m_prefix_list.at(i)->lang == lang)
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

QString ResourceFile::relativePath(const FilePath &abs_path) const
{
    QTC_ASSERT(!m_filePath.isEmpty(), return abs_path.path());
    QTC_CHECK(abs_path.isSameDevice(m_filePath));

    FilePath abs_path_on_device = m_filePath.withNewPath(abs_path.path());
    return abs_path_on_device.relativePathFromDir(m_filePath.parentDir());
}

FilePath ResourceFile::absolutePath(const QString &rel_path) const
{
    return m_filePath.parentDir().resolvePath(rel_path);
}

void ResourceFile::orderList()
{
    for (Prefix *p : std::as_const(m_prefix_list)) {
        std::sort(p->file_list.begin(), p->file_list.end(), [&](File *f1, File *f2) {
            return *f1 < *f2;
        });
    }

    if (!save())
        m_error_message = Tr::tr("Cannot save file.");
}

bool ResourceFile::contains(const QString &prefix, const QString &lang, const QString &file) const
{
    int pref_idx = indexOfPrefix(prefix, lang);
    if (pref_idx == -1)
        return false;
    if (file.isEmpty())
        return true;
    Q_ASSERT(pref_idx >= 0 && pref_idx < m_prefix_list.count());
    Prefix * const p = m_prefix_list.at(pref_idx);
    Q_ASSERT(p);
    File equalFile(p, absolutePath(file));
    return containsFile(p->file_list, &equalFile);
}

bool ResourceFile::contains(int pref_idx, const QString &file) const
{
    Q_ASSERT(pref_idx >= 0 && pref_idx < m_prefix_list.count());
    Prefix * const p = m_prefix_list.at(pref_idx);
    File equalFile(p, absolutePath(file));
    return containsFile(p->file_list, &equalFile);
}

/*static*/ QString ResourceFile::fixPrefix(const QString &prefix)
{
    const QChar slash = QLatin1Char('/');
    QString result = QString(slash);
    for (const QChar c : prefix) {
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

FilePath ResourceFile::file(int prefix_idx, int file_idx) const
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

ResourceModel::ResourceModel()
{
    static QIcon resourceFolderIcon = FileIconProvider::directoryIcon(QLatin1String(ProjectExplorer::Constants::FILEOVERLAY_QRC));
    m_prefixIcon = resourceFolderIcon;
}

void ResourceModel::setDirty(bool b)
{
    if (b)
        emit contentsChanged();
    if (b == m_dirty)
        return;

    m_dirty = b;
    emit dirtyChanged(b);
}

QModelIndex ResourceModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0)
        return QModelIndex();

    void *internalPointer = nullptr;
    if (parent.isValid()) {
        void * const pip = parent.internalPointer();
        if (!pip)
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
    if (!internalPointer)
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

QString ResourceModel::errorMessage() const
{
    return m_resource_file.errorMessage();
}

QList<QModelIndex> ResourceModel::nonExistingFiles() const
{
    QList<QModelIndex> files;
    int prefixCount = rowCount(QModelIndex());
    for (int i = 0; i < prefixCount; ++i) {
        QModelIndex prefix = index(i, 0, QModelIndex());
        int fileCount = rowCount(prefix);
        for (int j = 0; j < fileCount; ++j) {
            QModelIndex fileIndex = index(j, 0, prefix);
            FilePath fileName = file(fileIndex);
            if (!fileName.exists())
                files << fileIndex;
        }
    }
    return files;
}

void ResourceModel::orderList()
{
    m_resource_file.orderList();
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

bool ResourceModel::hasIconFileExtension(const QString &path)
{
    static QStringList ext_list;
    if (ext_list.isEmpty()) {
        const QList<QByteArray> _ext_list = QImageReader::supportedImageFormats();
        for (const QByteArray &ext : _ext_list) {
            QString dotExt = QString(QLatin1Char('.'));
            dotExt  += QString::fromLatin1(ext);
            ext_list.append(dotExt);
        }
    }

    for (const QString &ext : std::as_const(ext_list)) {
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
            if (file->icon.isNull()) {
                const FilePath path = file->name;
                if (hasIconFileExtension(path.path()))
                    file->icon = QIcon(path.toFSPathString());
                else
                    file->icon = FileIconProvider::icon(path);
            }
            if (!file->icon.isNull())
                result = file->icon;

        } else {
            result = m_prefixIcon;
        }
        break;
    case Qt::EditRole:
        if (isFileNode)
            result = m_resource_file.relativePath(file->name);
        break;
    case Qt::ForegroundRole:
        if (isFileNode) {
            // File node
            if (!file->exists())
                result = Utils::creatorColor(Theme::TextColorError);
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

    const QDir baseDir = filePath().toFileInfo().absoluteDir();
    FilePath newFileName = FilePath::fromUserInput(baseDir.absoluteFilePath(value.toString()));

    if (newFileName.isEmpty())
        return false;

    if (!newFileName.isChildOf(filePath().absolutePath()))
        return false;

    return renameFile(file(index), newFileName);
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
        if (!f->alias.isEmpty())
            file = f->alias;
        else
            file = f->name.path();
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

FilePath ResourceModel::file(const QModelIndex &index) const
{
    if (!index.isValid() || !index.parent().isValid())
        return {};
    return m_resource_file.file(index.parent().row(), index.row());
}

QModelIndex ResourceModel::getIndex(const QString &prefix, const QString &lang, const QString &file)
{
    if (prefix.isEmpty())
        return QModelIndex();

    const int pref_idx = m_resource_file.indexOfPrefix(prefix, lang);
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
    for ( ; m_resource_file.contains(prefix, QString()); i++)
        prefix = format.arg(i);

    i = rowCount(QModelIndex());
    beginInsertRows(QModelIndex(), i, i);
    m_resource_file.addPrefix(prefix, QString());
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
        for (const QString &file : fileNames) {
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

    const QStringList unique_list = existingFilesSubtracted(prefixIndex, fileNames);

    if (unique_list.isEmpty())
        return;

    const int cnt = m_resource_file.fileCount(prefixIndex);
    beginInsertRows(prefix_model_idx, cnt, cnt + unique_list.count() - 1); // ### FIXME

    for (const QString &file : unique_list)
        m_resource_file.addFile(prefixIndex, file);

    const QFileInfo fi(unique_list.last());
    m_lastResourceDir = fi.absolutePath();

    endInsertRows();
    setDirty(true);

    firstFile = cnt;
    lastFile = cnt + unique_list.count() - 1;

    Core::VcsManager::promptToAdd(filePath(), FilePaths::fromStrings(fileNames));
}


void ResourceModel::insertPrefix(int prefixIndex, const QString &prefix,
        const QString &lang)
{
    beginInsertRows(QModelIndex(), prefixIndex, prefixIndex);
    m_resource_file.addPrefix(prefix, lang, prefixIndex);
    endInsertRows();
    setDirty(true);
}

void ResourceModel::insertFile(
    int prefixIndex, int fileIndex, const QString &fileName, const QString &alias)
{
    const QModelIndex parent = index(prefixIndex, 0, QModelIndex());
    beginInsertRows(parent, fileIndex, fileIndex);
    m_resource_file.addFile(prefixIndex, fileName, fileIndex);
    m_resource_file.replaceAlias(prefixIndex, fileIndex, alias);
    endInsertRows();
    setDirty(true);
}

bool ResourceModel::renameFile(const FilePath &filePath, const FilePath &newFilePath)
{
    bool success = m_resource_file.renameFile(filePath, newFilePath);
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
    if (!m_resource_file.replacePrefix(prefix_idx, prefix))
        return;
    emit dataChanged(prefix_model_idx, prefix_model_idx);
    setDirty(true);
}

void ResourceModel::changeLang(const QModelIndex &model_idx, const QString &lang)
{
    if (!model_idx.isValid())
        return;

    const QModelIndex prefix_model_idx = prefixIndex(model_idx);
    const int prefix_idx = model_idx.row();
    if (!m_resource_file.replaceLang(prefix_idx, lang))
        return;
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

Result<> ResourceModel::reload()
{
    beginResetModel();
    Result<> result = m_resource_file.load();
    if (result.has_value())
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
        return m_resource_file.filePath().toFSPathString();
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
        return nullptr;

    QString prefix, file;
    getItem(indexes.front(), prefix, file);
    if (prefix.isEmpty() || file.isEmpty())
        return nullptr;

    // DnD format of Designer 4.4
    QDomDocument doc;
    QDomElement elem = doc.createElement(QLatin1String("resource"));
    elem.setAttribute(QLatin1String("type"), QLatin1String("image"));
    elem.setAttribute(QLatin1String("file"), resourcePath(prefix, file));
    doc.appendChild(elem);

    auto rc = new QMimeData;
    rc->setText(doc.toString());
    return rc;
}


/*!
    \class FileEntryBackup

    Backups a file node.
*/
class FileEntryBackup : public EntryBackup
{
private:
    int m_fileIndex;
    QString m_alias;

public:
    FileEntryBackup(ResourceModel &model, int prefixIndex, int fileIndex,
            const QString &fileName, const QString &alias)
        : EntryBackup(model, prefixIndex, fileName), m_fileIndex(fileIndex), m_alias(alias)
    { }

    void restore() const override;
};

void FileEntryBackup::restore() const
{
    m_model->insertFile(m_prefixIndex, m_fileIndex, m_name, m_alias);
}

/*!
    \class PrefixEntryBackup

    Backups a prefix node including children.
*/
class PrefixEntryBackup : public EntryBackup
{
private:
    QString m_language;
    QList<FileEntryBackup> m_files;

public:
    PrefixEntryBackup(ResourceModel &model, int prefixIndex, const QString &prefix,
            const QString &language, const QList<FileEntryBackup> &files)
            : EntryBackup(model, prefixIndex, prefix), m_language(language), m_files(files) { }
    void restore() const override;
};

void PrefixEntryBackup::restore() const
{
    m_model->insertPrefix(m_prefixIndex, m_name, m_language);
    for (const FileEntryBackup &entry : m_files) {
        entry.restore();
    }
}

RelativeResourceModel::RelativeResourceModel() = default;

Qt::ItemFlags RelativeResourceModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = ResourceModel::flags(index);
    if ((rc & Qt::ItemIsEnabled) && m_resourceDragEnabled)
        rc |= Qt::ItemIsDragEnabled;
    return rc;
}

EntryBackup * RelativeResourceModel::removeEntry(const QModelIndex &index)
{
    const QModelIndex prefixIndex = this->prefixIndex(index);
    const bool isPrefixNode = (prefixIndex == index);

    // Create backup, remove, return backup
    if (isPrefixNode) {
        QString dummy;
        QString prefixBackup;
        getItem(index, prefixBackup, dummy);
        const QString languageBackup = lang(index);
        const int childCount = rowCount(index);
        QList<FileEntryBackup> filesBackup;
        for (int i = 0; i < childCount; i++) {
            const QModelIndex childIndex = this->index(i, 0, index);
            const FilePath fileNameBackup = file(childIndex);
            const QString aliasBackup = alias(childIndex);
            FileEntryBackup entry(*this, index.row(), i, fileNameBackup.path(), aliasBackup);
            filesBackup << entry;
        }
        deleteItem(index);
        return new PrefixEntryBackup(*this, index.row(), prefixBackup, languageBackup, filesBackup);
    } else {
        const FilePath fileNameBackup = file(index);
        const QString aliasBackup = alias(index);
        if (!fileNameBackup.exists()) {
            deleteItem(index);
            return new FileEntryBackup(*this, prefixIndex.row(), index.row(), fileNameBackup.path(), aliasBackup);
        }
        RemoveFileDialog removeFileDialog(fileNameBackup);
        if (removeFileDialog.exec() == QDialog::Accepted) {
            deleteItem(index);
            Core::FileUtils::removeFiles({fileNameBackup},
                                         removeFileDialog.isDeleteFileChecked());
            return new FileEntryBackup(*this, prefixIndex.row(), index.row(), fileNameBackup.path(), aliasBackup);
        }
        return nullptr;
    }
}

} // ResourceEditor::Internal
