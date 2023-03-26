// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../filepath.h"

#include <QtCore/private/qabstractfileengine_p.h>

#include <QTemporaryFile>

namespace Utils {
namespace Internal {

class FSEngineImpl : public QAbstractFileEngine
{
public:
    FSEngineImpl(FilePath filePath);
    ~FSEngineImpl();

public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    bool open(QIODeviceBase::OpenMode openMode,
              std::optional<QFile::Permissions> permissions = std::nullopt) override;
    bool mkdir(const QString &dirName, bool createParentDirectories,
              std::optional<QFile::Permissions> permissions = std::nullopt) const override;
#else
    bool open(QIODevice::OpenMode openMode) override;
    bool mkdir(const QString &dirName, bool createParentDirectories) const override;
#endif
    bool close() override;
    bool flush() override;
    bool syncToDisk() override;
    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    bool isSequential() const override;
    bool remove() override;
    bool copy(const QString &newName) override;
    bool rename(const QString &newName) override;
    bool renameOverwrite(const QString &newName) override;
    bool link(const QString &newName) override;
    bool rmdir(const QString &dirName, bool recurseParentDirectories) const override;
    bool setSize(qint64 size) override;
    bool caseSensitive() const override;
    bool isRelativePath() const override;
    QStringList entryList(QDir::Filters filters, const QStringList &filterNames) const override;
    FileFlags fileFlags(FileFlags type) const override;
    bool setPermissions(uint perms) override;
    QByteArray id() const override;
    QString fileName(FileName file) const override;
    uint ownerId(FileOwner) const override;
    QString owner(FileOwner) const override;
    bool setFileTime(const QDateTime &newDate, FileTime time) override;
    QDateTime fileTime(FileTime time) const override;
    void setFileName(const QString &file) override;
    int handle() const override;
    bool cloneTo(QAbstractFileEngine *target) override;
    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames) override;
    Iterator *endEntryList() override;
    qint64 read(char *data, qint64 maxlen) override;
    qint64 readLine(char *data, qint64 maxlen) override;
    qint64 write(const char *data, qint64 len) override;
    bool extension(Extension extension,
                   const ExtensionOption *option,
                   ExtensionReturn *output) override;
    bool supportsExtension(Extension extension) const override;

private:
    void ensureStorage();

    FilePath m_filePath;
    QTemporaryFile *m_tempStorage = nullptr;

    bool m_hasChangedContent{false};
};

} // namespace Internal
} // namespace Utils
