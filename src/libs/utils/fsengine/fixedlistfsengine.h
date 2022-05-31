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
            return m_filePath.baseName();
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
