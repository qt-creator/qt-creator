/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppfileiterationorder.h"

#include <utils/qtcassert.h>

namespace CppTools {

FileIterationOrder::Entry::Entry(const QString &filePath,
                                 const QString &projectPartId,
                                 int commonPrefixLength,
                                 int commonProjectPartPrefixLength)
    : filePath(filePath)
    , projectPartId(projectPartId)
    , commonFilePathPrefixLength(commonPrefixLength)
    , commonProjectPartPrefixLength(commonProjectPartPrefixLength)
{}

namespace {

bool cmpPrefixLengthAndText(int prefixLength1, int prefixLength2)
{
    return prefixLength1 > prefixLength2;
}

bool cmpLessFilePath(const FileIterationOrder::Entry &first,
                     const FileIterationOrder::Entry &second)
{
    return cmpPrefixLengthAndText(first.commonFilePathPrefixLength,
                                  second.commonFilePathPrefixLength);
}

bool cmpLessProjectPartId(const FileIterationOrder::Entry &first,
                          const FileIterationOrder::Entry &second)
{
    return cmpPrefixLengthAndText(first.commonProjectPartPrefixLength,
                                  second.commonProjectPartPrefixLength);
}

bool hasProjectPart(const FileIterationOrder::Entry &entry)
{
    return !entry.projectPartId.isEmpty();
}

} // anonymous namespace

bool operator<(const FileIterationOrder::Entry &first, const FileIterationOrder::Entry &second)
{
    if (hasProjectPart(first)) {
        if (hasProjectPart(second)) {
            if (first.projectPartId == second.projectPartId)
                return cmpLessFilePath(first, second);
            else
                return cmpLessProjectPartId(first, second);
        } else {
            return true;
        }
    } else {
        if (hasProjectPart(second))
            return false;
        else
            return cmpLessFilePath(first, second);
    }
}

FileIterationOrder::FileIterationOrder()
{
}

FileIterationOrder::FileIterationOrder(const QString &referenceFilePath,
                                       const QString &referenceProjectPartId)
{
    setReference(referenceFilePath, referenceProjectPartId);
}

void FileIterationOrder::setReference(const QString &filePath,
                                      const QString &projectPartId)
{
    m_referenceFilePath = filePath;
    m_referenceProjectPartId = projectPartId;
}

bool FileIterationOrder::isValid() const
{
    return !m_referenceFilePath.isEmpty();
}

static int commonPrefixLength(const QString &filePath1, const QString &filePath2)
{
    const auto mismatches = std::mismatch(filePath1.begin(),
                                          filePath1.end(),
                                          filePath2.begin());
    return mismatches.first - filePath1.begin();
}

FileIterationOrder::Entry FileIterationOrder::createEntryFromFilePath(
        const QString &filePath,
        const QString &projectPartId) const
{
    const int filePrefixLength = commonPrefixLength(m_referenceFilePath, filePath);
    const int projectPartPrefixLength = commonPrefixLength(m_referenceProjectPartId, projectPartId);
    return Entry(filePath, projectPartId, filePrefixLength, projectPartPrefixLength);
}

void FileIterationOrder::insert(const QString &filePath, const QString &projectPartId)
{
    const Entry entry = createEntryFromFilePath(filePath, projectPartId);
    m_set.insert(entry);
}

void FileIterationOrder::remove(const QString &filePath, const QString &projectPartId)
{
    const auto needleElement = createEntryFromFilePath(filePath, projectPartId);
    const auto range = m_set.equal_range(needleElement);

    const auto toRemove = std::find_if(range.first, range.second, [filePath] (const Entry &entry) {
        return entry.filePath == filePath;
    });
    QTC_ASSERT(toRemove != range.second, return);
    m_set.erase(toRemove);
}

QStringList FileIterationOrder::toStringList() const
{
    QStringList result;

    for (const auto &entry : m_set)
        result.append(entry.filePath);

    return result;
}

} // namespace CppTools
