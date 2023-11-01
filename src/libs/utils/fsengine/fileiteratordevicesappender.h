// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../filepath.h"
#include "../hostosinfo.h"

#include <QtCore/private/qabstractfileengine_p.h>

namespace Utils {
namespace Internal {

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

class FileIteratorWrapper : public QAbstractFileEngineIterator
{
    enum class State {
        NotIteratingRoot,
        IteratingRoot,
        BaseIteratorEnd,
        Ended,
    };

public:
    FileIteratorWrapper(std::unique_ptr<QAbstractFileEngineIterator> &&baseIterator,
                        QDir::Filters filters,
                        const QStringList &filterNames)
        : QAbstractFileEngineIterator(filters, filterNames)
        , m_baseIterator(std::move(baseIterator))
    {}

public:
    QString next() override
    {
        if (m_status == State::Ended)
            return QString();

        setPath();
        checkStatus();

        if (m_status == State::BaseIteratorEnd) {
            m_status = State::Ended;
            return "__qtc__devices__";
        }

        return m_baseIterator->next();
    }
    bool hasNext() const override
    {
        if (m_status == State::Ended)
            return false;

        setPath();
        checkStatus();

        if (m_status == State::BaseIteratorEnd)
            return true;

        return m_baseIterator->hasNext();
    }
    QString currentFileName() const override
    {
        if (m_status == State::Ended)
            return FilePath::specialRootPath();

        setPath();
        checkStatus();
        return m_baseIterator->currentFileName();
    }
    QFileInfo currentFileInfo() const override
    {
        if (m_status == State::Ended)
            return QFileInfo(FilePath::specialRootPath());
        setPath();
        checkStatus();
        return m_baseIterator->currentFileInfo();
    }

private:
    void setPath() const
    {
        if (!m_hasSetPath) {
            // path() can be "/somedir/.." so we need to clean it first.
            // We only need QDir::cleanPath here, as the path is always
            // a fs engine path and will not contain scheme:// etc.
            const QString p = QDir::cleanPath(path());
            if (p.compare(HostOsInfo::root().path(), Qt::CaseInsensitive) == 0)
                m_status = State::IteratingRoot;

            ((*m_baseIterator).*get(QAFEITag()))(p);
            m_hasSetPath = true;
        }
    }

    void checkStatus() const
    {
        if (m_status == State::NotIteratingRoot) {
            return;
        }
        if (m_status == State::IteratingRoot) {
            if (m_baseIterator->hasNext() == false) {
                m_status = State::BaseIteratorEnd;
            }
        }
    }

private:
    std::unique_ptr<QAbstractFileEngineIterator> m_baseIterator;
    mutable bool m_hasSetPath{false};
    mutable State m_status{State::NotIteratingRoot};
};

} // namespace Internal
} // namespace Utils
