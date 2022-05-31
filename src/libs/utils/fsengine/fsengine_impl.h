/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "../filepath.h"

#include <QtCore/private/qabstractfileengine_p.h>

#include <QTemporaryFile>

namespace Utils {
namespace Internal {

class FSEngineImpl : public QAbstractFileEngine
{
public:
    FSEngineImpl(FilePath filePath);

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
    FilePath m_filePath;
    QTemporaryFile m_tempStorage;

    bool m_hasChangedContent{false};
};

} // namespace Internal
} // namespace Utils
