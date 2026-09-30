// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "testtreeitem.h"

#include <cplusplus/CppDocument.h>
#include <cppeditor/cppworkingcopy.h>

#include <functional>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE
template <class T>
class QPromise;
class QRegularExpression;
QT_END_NAMESPACE

namespace Autotest {

class ITestFramework;

class TestParseResult
{
public:
    explicit TestParseResult(ITestFramework *framework) : framework(framework) {}
    virtual ~TestParseResult() { qDeleteAll(children); }

    virtual TestTreeItem *createTestTreeItem() const = 0;

    QList<TestParseResult *> children;
    ITestFramework *framework;
    TestTreeItem::Type itemType = TestTreeItem::Root;
    QString displayName;
    Utils::FilePath fileName;
    Utils::FilePath proFile;
    QString name;
    int line = 0;
    int column = 0;
};

// Read-only once created. Scan threads may outlive their scan, so parsers
// must not keep any of this in members.
class CppParseContext
{
public:
    CPlusPlus::Snapshot cppSnapshot;
    CppEditor::WorkingCopy workingCopy;
};

using CppParseContextPtr = std::shared_ptr<const CppParseContext>;

// Called from the scan threads, once per file. Returns true if the file has been handled.
using DocumentProcessor
    = std::function<bool(QPromise<TestParseResultPtr> &promise, const Utils::FilePath &fileName)>;

class ITestParser
{
public:
    explicit ITestParser(ITestFramework *framework) : m_framework(framework) {}
    virtual ~ITestParser() = default;

    virtual DocumentProcessor init(const QSet<Utils::FilePath> &filesToParse, bool fullParse) = 0;

    virtual QStringList supportedExtensions() const { return {}; }

    ITestFramework *framework() const { return m_framework; }

private:
    ITestFramework *m_framework;
};

class CppParser : public ITestParser
{
public:
    explicit CppParser(ITestFramework *framework);

    static bool selectedForBuilding(const Utils::FilePath &fileName);
    static QByteArray getFileContent(const CppParseContext &context,
                                     const Utils::FilePath &filePath);

    static CPlusPlus::Document::Ptr document(const CppParseContext &context,
                                             const Utils::FilePath &fileName);

    static bool precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                                          const Utils::FilePath &filePath,
                                          const QString &headerFilePath);
    static bool precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                                          const Utils::FilePath &filePath,
                                          const QRegularExpression &headerFileRegex);
    // returns all files of the startup project whose ProjectPart has the given \a macroName
    // set as a project define
    static std::optional<QSet<Utils::FilePath>> filesContainingMacro(const QByteArray &macroName);

    static void clearCaches();

protected:
    static CppParseContextPtr createContext();
    static void fillContext(CppParseContext &context);
};

} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::TestParseResultPtr)
