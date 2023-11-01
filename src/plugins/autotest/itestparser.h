// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "testtreeitem.h"

#include <cplusplus/CppDocument.h>
#include <cppeditor/cppworkingcopy.h>
#include <qmljs/qmljsdocument.h>

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

    QVector<TestParseResult *> children;
    ITestFramework *framework;
    TestTreeItem::Type itemType = TestTreeItem::Root;
    QString displayName;
    Utils::FilePath fileName;
    Utils::FilePath proFile;
    QString name;
    int line = 0;
    int column = 0;
};

using TestParseResultPtr = QSharedPointer<TestParseResult>;

class ITestParser
{
public:
    explicit ITestParser(ITestFramework *framework) : m_framework(framework) {}
    virtual ~ITestParser() { }
    virtual void init(const QSet<Utils::FilePath> &filesToParse, bool fullParse) = 0;
    virtual bool processDocument(QPromise<TestParseResultPtr> &futureInterface,
                                 const Utils::FilePath &fileName) = 0;

    virtual QStringList supportedExtensions() const { return {}; }

    virtual void release() = 0;

    ITestFramework *framework() const { return m_framework; }

private:
    ITestFramework *m_framework;
};

class CppParser : public ITestParser
{
public:
    explicit CppParser(ITestFramework *framework);
    void init(const QSet<Utils::FilePath> &filesToParse, bool fullParse) override;
    static bool selectedForBuilding(const Utils::FilePath &fileName);
    QByteArray getFileContent(const Utils::FilePath &filePath) const;
    void release() override;

    CPlusPlus::Document::Ptr document(const Utils::FilePath &fileName);

    static bool precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                                          const Utils::FilePath &filePath,
                                          const QString &headerFilePath);
    static bool precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                                          const Utils::FilePath &filePath,
                                          const QRegularExpression &headerFileRegex);
    // returns all files of the startup project whose ProjectPart has the given \a macroName
    // set as a project define
    static std::optional<QSet<Utils::FilePath>> filesContainingMacro(const QByteArray &macroName);

protected:
    CPlusPlus::Snapshot m_cppSnapshot;
    CppEditor::WorkingCopy m_workingCopy;
};

} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::TestParseResultPtr)
