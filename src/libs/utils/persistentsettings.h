// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "store.h"

#include <QVariant>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

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

    bool save(const Store &data, QString *errorString) const;
#ifdef QT_GUI_LIB
    bool save(const Store &data, QWidget *parent) const;
#endif

    FilePath fileName() const;

    void setContents(const Store &data);

private:
    bool write(const Store &data, QString *errorString) const;

    const FilePath m_fileName;
    const QString m_docType;
    mutable Store m_savedData;
};

} // namespace Utils
