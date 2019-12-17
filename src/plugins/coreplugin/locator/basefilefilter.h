/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "ilocatorfilter.h"

#include <utils/fileutils.h>

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

    class CORE_EXPORT ListIterator : public Iterator {
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
    void accept(LocatorFilterEntry selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;

protected:
    void setFileIterator(Iterator *iterator);
    QSharedPointer<Iterator> fileIterator();

private:
    MatchLevel matchLevelFor(const QRegularExpressionMatch &match, const QString &matchText) const;
    void updatePreviousResultData();

    Internal::BaseFileFilterPrivate *d = nullptr;
};

} // namespace Core
