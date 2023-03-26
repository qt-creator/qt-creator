// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDateTime>

namespace Utils {

struct FilePathInfo
{
    // Copy of QAbstractFileEngine::FileFlags so we don't need to include private headers.
    enum FileFlag {
        //perms (overlaps the QFile::Permission)
        ReadOwnerPerm = 0x4000,  // 0x100
        WriteOwnerPerm = 0x2000, // 0x80
        ExeOwnerPerm = 0x1000,   // 0x40
        ReadUserPerm = 0x0400,
        WriteUserPerm = 0x0200,
        ExeUserPerm = 0x0100,
        ReadGroupPerm = 0x0040,  // 0x20
        WriteGroupPerm = 0x0020, // 0x10
        ExeGroupPerm = 0x0010,   // 0x8
        ReadOtherPerm = 0x0004,  // 0x4
        WriteOtherPerm = 0x0002, // 0x2
        ExeOtherPerm = 0x0001,   // 0x1

        //types
        LinkType = 0x10000,      // 0xa000
        FileType = 0x20000,      // 0x8000
        DirectoryType = 0x40000, // 0x4000
        BundleType = 0x80000,

        //flags
        HiddenFlag = 0x0100000,
        LocalDiskFlag = 0x0200000, // 0x6000
        ExistsFlag = 0x0400000,
        RootFlag = 0x0800000,
        Refresh = 0x1000000,

        //masks
        PermsMask = 0x0000FFFF,
        TypesMask = 0x000F0000,
        FlagsMask = 0x0FF00000,
        FileInfoAll = FlagsMask | PermsMask | TypesMask
    };

    Q_DECLARE_FLAGS(FileFlags, FileFlag)

    bool operator==(const FilePathInfo &other) const
    {
        return fileSize == other.fileSize && fileFlags == other.fileFlags
               && lastModified == other.lastModified;
    }

    qint64 fileSize = 0;
    FileFlags fileFlags;
    QDateTime lastModified;
};

} // namespace Utils
