// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "result.h"
#include "store.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT PersistentSettingsReader
{
public:
    PersistentSettingsReader();
    QVariant restoreValue(const Key &variable, const QVariant &defaultValue = {}) const;
    Store restoreValues() const;
    bool load(const FilePath &fileName);
    FilePath filePath();

private:
    QVariantMap m_valueMap;
    FilePath m_filePath;
};

class QTCREATOR_UTILS_EXPORT PersistentSettingsWriter
{
public:
    PersistentSettingsWriter(const FilePath &fileName, const QString &docType);

    Result<> save(const Store &data, bool showError = true) const;

    FilePath fileName() const;

    void setContents(const Store &data);

private:
    Result<> write(const Store &data) const;

    const FilePath m_fileName;
    const QString m_docType;
    mutable Store m_savedData;
};

} // namespace Utils
