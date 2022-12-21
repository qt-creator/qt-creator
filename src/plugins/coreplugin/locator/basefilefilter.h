// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

#include <utils/filepath.h>

#include <QSharedPointer>

namespace Core {

namespace Internal { class BaseFileFilterPrivate; }

class CORE_EXPORT BaseFileFilter : public ILocatorFilter
{
    Q_OBJECT

public:
    class CORE_EXPORT Iterator {
    public:
        virtual ~Iterator();
        virtual void toFront() = 0;
        virtual bool hasNext() const = 0;
        virtual Utils::FilePath next() = 0;
        virtual Utils::FilePath filePath() const = 0;
    };

    class CORE_EXPORT ListIterator final : public Iterator {
    public:
        ListIterator(const Utils::FilePaths &filePaths);

        void toFront() override;
        bool hasNext() const override;
        Utils::FilePath next() override;
        Utils::FilePath filePath() const override;

    private:
        Utils::FilePaths m_filePaths;
        Utils::FilePaths::const_iterator m_pathPosition;
    };

    BaseFileFilter();
    ~BaseFileFilter() override;
    void prepareSearch(const QString &entry) override;
    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                         const QString &entry) override;
    void accept(const LocatorFilterEntry &selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;
    static void openEditorAt(const LocatorFilterEntry &selection);

protected:
    void setFileIterator(Iterator *iterator);
    QSharedPointer<Iterator> fileIterator();

private:
    static MatchLevel matchLevelFor(const QRegularExpressionMatch &match,
                                    const QString &matchText);
    void updatePreviousResultData();

    Internal::BaseFileFilterPrivate *d = nullptr;
};

} // namespace Core
