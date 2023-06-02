// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../filepath.h"
#include "../stringutils.h"

#include <QFileInfo>
#include <QString>

#include <QtCore/private/qabstractfileengine_p.h>

namespace Utils {
namespace Internal {

class DirIterator : public QAbstractFileEngineIterator
{
public:
    DirIterator(FilePaths paths)
        : QAbstractFileEngineIterator({}, {})
        , m_filePaths(std::move(paths))
        , it(m_filePaths.begin())
    {}

    // QAbstractFileEngineIterator interface
public:
    QString next() override
    {
        if (it == m_filePaths.end())
            return QString();
        const QString r = chopIfEndsWith(it->toFSPathString(), '/');
        ++it;
        return r;
    }

    bool hasNext() const override { return !m_filePaths.empty() && m_filePaths.end() != it + 1; }

    QString currentFileName() const override
    {
        const QString result = it->fileName();
        if (result.isEmpty() && !it->host().isEmpty()) {
            return it->host().toString();
        }
        return chopIfEndsWith(result, '/');
    }

    QFileInfo currentFileInfo() const override
    {
        return QFileInfo(chopIfEndsWith(it->toFSPathString(), '/'));
    }

private:
    const FilePaths m_filePaths;
    FilePaths::const_iterator it;
};

} // namespace Internal

} // namespace Utils
