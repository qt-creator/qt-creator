// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <QStringList>

namespace CppEditor {
class CppLocatorData; // FIXME: Belongs in namespace Internal

namespace Internal {

/**
 * This class extends the JS features in our macro expander.
 */
class CppToolsJsExtension : public QObject
{
    Q_OBJECT

public:
    explicit CppToolsJsExtension(CppLocatorData *locatorData, QObject *parent = nullptr)
        : QObject(parent), m_locatorData(locatorData) { }

    // Generate header guard:
    Q_INVOKABLE QString headerGuard(const QString &in) const;

    // Generate license template:
    Q_INVOKABLE QString licenseTemplate() const;

    // Use #pragma once:
    Q_INVOKABLE bool usePragmaOnce() const;

    // Work with classes:
    Q_INVOKABLE QStringList namespaces(const QString &klass) const;
    Q_INVOKABLE bool hasNamespaces(const QString &klass) const;
    Q_INVOKABLE QString className(const QString &klass) const;
    // Fix the filename casing as configured in C++/File Naming:
    Q_INVOKABLE QString classToFileName(const QString &klass,
                                        const QString &extension) const;
    Q_INVOKABLE QString classToHeaderGuard(const QString &klass, const QString &extension) const;
    Q_INVOKABLE QString openNamespaces(const QString &klass) const;
    Q_INVOKABLE QString closeNamespaces(const QString &klass) const;
    Q_INVOKABLE bool hasQObjectParent(const QString &klassName) const;

    // Find header file for class.
    Q_INVOKABLE QString includeStatement(
            const QString &fullyQualifiedClassName,
            const QString &suffix,
            const QStringList &specialClasses,
            const QString &pathOfIncludingFile
            );

    // File suffixes:
    Q_INVOKABLE QString cxxHeaderSuffix() const;
    Q_INVOKABLE QString cxxSourceSuffix() const;

private:
    CppLocatorData * const m_locatorData;
};

} // namespace Internal
} // namespace CppEditor
