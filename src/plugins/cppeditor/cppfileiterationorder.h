// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <QStringList>

#include <set>

namespace CppEditor {

class CPPEDITOR_EXPORT FileIterationOrder {
public:
    struct Entry {
        Entry(const QString &filePath,
              const QString &projectPartId = QString(),
              int commonFilePathPrefixLength = 0,
              int commonProjectPartPrefixLength = 0);

        friend CPPEDITOR_EXPORT bool operator<(const Entry &first, const Entry &second);

        const QString filePath;
        const QString projectPartId;
        int commonFilePathPrefixLength = 0;
        int commonProjectPartPrefixLength = 0;
    };

    FileIterationOrder();
    FileIterationOrder(const QString &referenceFilePath,
                       const QString &referenceProjectPartId);

    void setReference(const QString &filePath, const QString &projectPartId);
    bool isValid() const;

    void insert(const QString &filePath, const QString &projectPartId = QString());
    void remove(const QString &filePath, const QString &projectPartId);
    QStringList toStringList() const;

private:
    Entry createEntryFromFilePath(const QString &filePath,
                                  const QString &projectPartId) const;

private:
    QString m_referenceFilePath;
    QString m_referenceProjectPartId;
    std::multiset<Entry> m_set;
};

CPPEDITOR_EXPORT bool operator<(const FileIterationOrder::Entry &first,
                                const FileIterationOrder::Entry &second);

} // namespace CppEditor
