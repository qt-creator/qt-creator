// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../filepath.h"
#include "../hostosinfo.h"

#include <QtCore/private/qabstractfileengine_p.h>
#include <QtGlobal>

namespace Utils {
namespace Internal {

class FileIteratorWrapper : public QAbstractFileEngineIterator
{
    enum class State {
        NotIteratingRoot,
        IteratingRoot,
        BaseIteratorEnd,
        Ended,
    };

public:
    FileIteratorWrapper(std::unique_ptr<QAbstractFileEngineIterator> &&baseIterator)
        : QAbstractFileEngineIterator(
            baseIterator->path(),
            baseIterator->filters(), baseIterator->nameFilters())
        , m_baseIterator(std::move(baseIterator))
    {
        // Can be called in the constructor since the iterator path
        // has already been set
        setStatus();
    }

public:
    bool advance() override
    {
        if (m_status == State::Ended)
            return false;

        const bool res = m_baseIterator->advance();
        if (m_status == State::IteratingRoot && !res) {
            // m_baseIterator finished, but we need to advance one last time, so that
            // currentFileName() returns FilePath::specialRootPath().
            m_status = State::Ended;
            return true;
        }

        return res;
    }
    QString currentFileName() const override
    {
        return m_status == State::Ended ? FilePath::specialRootPath()
                                        : m_baseIterator->currentFileName();
    }
    QFileInfo currentFileInfo() const override
    {
        return m_status == State::Ended ? QFileInfo(FilePath::specialRootPath())
                                        : m_baseIterator->currentFileInfo();
    }

private:
    QString setStatus() const
    {
        // path() can be "/somedir/.." so we need to clean it first.
        // We only need QDir::cleanPath here, as the path is always
        // a fs engine path and will not contain scheme:// etc.
        QString p = QDir::cleanPath(path());
        if (p.compare(HostOsInfo::root().path(), Qt::CaseInsensitive) == 0)
            m_status = State::IteratingRoot;
        return p;
    }

private:
    std::unique_ptr<QAbstractFileEngineIterator> m_baseIterator;
    mutable State m_status{State::NotIteratingRoot};
};

} // namespace Internal
} // namespace Utils
