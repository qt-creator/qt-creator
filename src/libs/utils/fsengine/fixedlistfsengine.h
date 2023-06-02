// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diriterator.h"

#include "../filepath.h"
#include "../stringutils.h"

#include <QtCore/private/qabstractfileengine_p.h>

namespace Utils {
namespace Internal {

class FixedListFSEngine : public QAbstractFileEngine
{
    const FilePath m_filePath;
    FilePaths m_children;

public:
    FixedListFSEngine(FilePath path, const FilePaths children)
        : m_filePath(std::move(path))
        , m_children(std::move(children))
    {
        m_children.prepend(m_filePath.pathAppended("."));
    }

    // QAbstractFileEngine interface
public:
    bool isRelativePath() const override { return false; }
    FileFlags fileFlags(FileFlags /*type*/) const override
    {
        return FileFlag::DirectoryType | FileFlag::ExistsFlag | FileFlag::ReadGroupPerm
               | FileFlag::ReadUserPerm | FileFlag::ReadOwnerPerm | FileFlag::ReadOtherPerm;
    }
    QString fileName(FileName file) const override
    {
        switch (file) {
        case QAbstractFileEngine::AbsoluteName:
        case QAbstractFileEngine::DefaultName:
        case QAbstractFileEngine::CanonicalName:
            return chopIfEndsWith(m_filePath.toString(), '/');
            break;
        case QAbstractFileEngine::BaseName:
            if (m_filePath.fileName().isEmpty())
                return m_filePath.host().toString();
            return m_filePath.fileName();
            break;
        case QAbstractFileEngine::PathName:
        case QAbstractFileEngine::AbsolutePathName:
        case QAbstractFileEngine::CanonicalPathName:
            return chopIfEndsWith(m_filePath.parentDir().toString(), '/');
            break;

        default:
        // case QAbstractFileEngine::LinkName:
        // case QAbstractFileEngine::BundleName:
        // case QAbstractFileEngine::JunctionName:
            return {};
            break;
        }

        return QAbstractFileEngine::fileName(file);
    }
    Iterator *beginEntryList(QDir::Filters /*filters*/, const QStringList & /*filterNames*/) override
    {
        return new DirIterator(m_children);
    }
};

} // namespace Internal
} // namespace Utils
