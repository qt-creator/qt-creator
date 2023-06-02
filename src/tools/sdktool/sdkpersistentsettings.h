// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QVariant>

class SdkPersistentSettingsReader
{
public:
    SdkPersistentSettingsReader();
    QVariant restoreValue(const QString &variable, const QVariant &defaultValue = QVariant()) const;
    QVariantMap restoreValues() const;
    bool load(const QString &fileName);

private:
    QMap<QString, QVariant> m_valueMap;
};

class SdkPersistentSettingsWriter
{
public:
    SdkPersistentSettingsWriter(const QString &fileName, const QString &docType);

    bool save(const QVariantMap &data, QString *errorString) const;

    QString fileName() const;

    void setContents(const QVariantMap &data);

private:
    bool write(const QVariantMap &data, QString *errorString) const;

    const QString m_fileName;
    const QString m_docType;
    mutable QMap<QString, QVariant> m_savedData;
};
