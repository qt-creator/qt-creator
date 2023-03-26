// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppfileiterationorder.h"

#include <utils/qtcassert.h>

using namespace Utils;

namespace CppEditor {

FileIterationOrder::Entry::Entry(const FilePath &filePath,
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

FileIterationOrder::FileIterationOrder() = default;

FileIterationOrder::FileIterationOrder(const FilePath &referenceFilePath,
                                       const QString &referenceProjectPartId)
{
    setReference(referenceFilePath, referenceProjectPartId);
}

void FileIterationOrder::setReference(const FilePath &filePath,
                                      const QString &projectPartId)
{
    m_referenceFilePath = filePath;
    m_referenceProjectPartId = projectPartId;
}

bool FileIterationOrder::isValid() const
{
    return !m_referenceFilePath.isEmpty();
}

static int commonPrefixLength(const QStringView filePath1, const QStringView filePath2)
{
    const auto mismatches = std::mismatch(filePath1.begin(), filePath1.end(),
                                          filePath2.begin(), filePath2.end());
    return mismatches.first - filePath1.begin();
}

FileIterationOrder::Entry FileIterationOrder::createEntryFromFilePath(
        const FilePath &filePath,
        const QString &projectPartId) const
{
    const int filePrefixLength = commonPrefixLength(m_referenceFilePath.pathView(), filePath.pathView());
    const int projectPartPrefixLength = commonPrefixLength(m_referenceProjectPartId, projectPartId);
    return Entry(filePath, projectPartId, filePrefixLength, projectPartPrefixLength);
}

void FileIterationOrder::insert(const FilePath &filePath, const QString &projectPartId)
{
    const Entry entry = createEntryFromFilePath(filePath, projectPartId);
    m_set.insert(entry);
}

void FileIterationOrder::remove(const FilePath &filePath, const QString &projectPartId)
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
        result.append(entry.filePath.toString());

    return result;
}

Utils::FilePaths FileIterationOrder::toFilePaths() const
{
    FilePaths result;

    for (const auto &entry : m_set)
        result.append(entry.filePath);

    return result;
}

} // namespace CppEditor
