// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../filepath.h"
#include "../hostosinfo.h"

#include <QtCore/private/qabstractfileengine_p.h>
#include <QtGlobal>

namespace Utils {
namespace Internal {

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
// Based on http://bloglitb.blogspot.com/2011/12/access-to-private-members-safer.htm
template<typename Tag, typename Tag::type M>
struct PrivateAccess
{
    friend typename Tag::type get(Tag) { return M; }
};

struct QAFEITag
{
    using type = void (QAbstractFileEngineIterator::*)(const QString &);
    friend type get(QAFEITag);
};

template struct PrivateAccess<QAFEITag, &QAbstractFileEngineIterator::setPath>;
#endif

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            baseIterator->path(),
#endif
            baseIterator->filters(), baseIterator->nameFilters())
        , m_baseIterator(std::move(baseIterator))
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        // Can be called in the constructor since the iterator path
        // has already been set
        setStatus();
#endif
    }

public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
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
#else
    QString next() override
    {
        if (m_status == State::Ended)
            return QString();

        if (m_status == State::BaseIteratorEnd) {
            m_status = State::Ended;
            return FilePath::specialRootName();
        }

        return m_baseIterator->next();
    }
    bool hasNext() const override
    {
        if (m_status == State::Ended)
            return false;

        setPath();

        const bool res = m_baseIterator->hasNext();
        if (m_status == State::IteratingRoot && !res) {
            // m_baseIterator finished, but we need to advance one last time, so that
            // e.g. next() and currentFileName() return FilePath::specialRootPath().
            m_status = State::BaseIteratorEnd;
            return true;
        }

        return res;
    }
#endif
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

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    void setPath() const
    {
        if (!m_hasSetPath) {
            const QString &p = setStatus();
            ((*m_baseIterator).*get(QAFEITag()))(p);
            m_hasSetPath = true;
        }
    }
#endif

private:
    std::unique_ptr<QAbstractFileEngineIterator> m_baseIterator;

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    mutable bool m_hasSetPath{false};
#endif

    mutable State m_status{State::NotIteratingRoot};
};

} // namespace Internal
} // namespace Utils
