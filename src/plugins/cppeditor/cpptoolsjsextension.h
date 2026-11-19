// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QStringList>

namespace Core::JsBind { class Registrar; }

namespace CppEditor {
class CppLocatorData; // FIXME: Belongs in namespace Internal

namespace Internal {

// Implements the functions of the "Cpp" JavaScript object in our macro expander.
class CppToolsJsExtension
{
public:
    explicit CppToolsJsExtension(CppLocatorData *locatorData) : m_locatorData(locatorData) { }

    // Generate header guard:
    QString headerGuard(const QString &in) const;

    // Generate license template:
    QString licenseTemplate() const;

    // Use #pragma once:
    bool usePragmaOnce() const;

    // Work with classes:
    QStringList namespaces(const QString &klass) const;
    bool hasNamespaces(const QString &klass) const;
    QString className(const QString &klass) const;
    // Fix the filename casing as configured in C++/File Naming:
    QString classToFileName(const QString &klass, const QString &extension) const;
    QString openNamespaces(const QString &klass) const;
    QString closeNamespaces(const QString &klass) const;
    bool hasQObjectParent(const QString &klassName) const;

    // Find header file for class.
    QString includeStatement(
            const QString &fullyQualifiedClassName,
            const QString &suffix,
            const QStringList &specialClasses,
            const QString &pathOfIncludingFile
            );

    // File suffixes:
    QString cxxHeaderSuffix() const;
    QString cxxSourceSuffix() const;

private:
    CppLocatorData * const m_locatorData;
};

// Registers the functions of the "Cpp" JavaScript object.
void registerCppToolsFunctions(Core::JsBind::Registrar &registrar, CppLocatorData *locatorData);

} // namespace Internal
} // namespace CppEditor
