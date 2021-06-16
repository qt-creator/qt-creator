/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qttestparser.h"
#include "qttestframework.h"
#include "qttestvisitors.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectpart.h>
#include <cplusplus/TypeOfExpression.h>
#include <utils/algorithm.h>

#include <QRegularExpressionMatchIterator>

namespace Autotest {
namespace Internal {

TestTreeItem *QtTestParseResult::createTestTreeItem() const
{
    if (itemType == TestTreeItem::Root)
        return nullptr;

    QtTestTreeItem *item = new QtTestTreeItem(framework, displayName, fileName, itemType);
    item->setProFile(proFile);
    item->setLine(line);
    item->setColumn(column);
    item->setInherited(m_inherited);
    item->setRunsMultipleTestcases(m_multiTest);

    for (const TestParseResult *funcParseResult : children)
        item->appendChild(funcParseResult->createTestTreeItem());
    return item;
}

static bool includesQtTest(const CPlusPlus::Document::Ptr &doc, const CPlusPlus::Snapshot &snapshot)
{
    static QStringList expectedHeaderPrefixes
            = Utils::HostOsInfo::isMacHost()
            ? QStringList({"QtTest.framework/Headers", "QtTest"}) : QStringList({"QtTest"});

    const QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    for (const CPlusPlus::Document::Include &inc : includes) {
        // TODO this short cut works only for #include <QtTest>
        // bad, as there could be much more different approaches
        if (inc.unresolvedFileName() == QString("QtTest")) {
            for (const QString &prefix : expectedHeaderPrefixes) {
                if (inc.resolvedFileName().endsWith(QString("%1/QtTest").arg(prefix)))
                    return true;
            }
        }
    }

    const QSet<QString> allIncludes = snapshot.allIncludesForDocument(doc->fileName());
    for (const QString &include : allIncludes) {
        for (const QString &prefix : expectedHeaderPrefixes) {
        if (include.endsWith(QString("%1/qtest.h").arg(prefix)))
            return true;
        }
    }

    for (const QString &prefix : expectedHeaderPrefixes) {
        if (CppParser::precompiledHeaderContains(snapshot,
                                                 Utils::FilePath::fromString(doc->fileName()),
                                                 QString("%1/qtest.h").arg(prefix))) {
            return true;
        }
    }
    return false;
}

static bool qtTestLibDefined(const Utils::FilePath &fileName)
{
    const QList<CppTools::ProjectPart::Ptr> parts =
            CppTools::CppModelManager::instance()->projectPart(fileName);
    if (parts.size() > 0) {
        return Utils::anyOf(parts.at(0)->projectMacros, [] (const ProjectExplorer::Macro &macro) {
            return macro.key == "QT_TESTLIB_LIB";
        });
    }
    return false;
}

TestCases QtTestParser::testCases(const CppTools::CppModelManager *modelManager,
                                  const Utils::FilePath &fileName) const
{
    const QByteArray &fileContent = getFileContent(fileName);
    CPlusPlus::Document::Ptr document = modelManager->document(fileName.toString());
    if (document.isNull())
        return {};

    const QList<CPlusPlus::Document::MacroUse> macros = document->macroUses();

    for (const CPlusPlus::Document::MacroUse &macro : macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (QTestUtils::isQTestMacro(name) && !macro.arguments().isEmpty()) {
            const CPlusPlus::Document::Block arg = macro.arguments().at(0);
            const QString name = QLatin1String(fileContent.mid(int(arg.bytesBegin()),
                                                               int(arg.bytesEnd() - arg.bytesBegin())));
            return { {name, false} };
        }
    }
    // check if one has used a self-defined macro or QTest::qExec() directly
    document = m_cppSnapshot.preprocessedDocument(fileContent, fileName);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    TestAstVisitor astVisitor(document, m_cppSnapshot);
    astVisitor.accept(ast);
    if (!astVisitor.testCases().isEmpty())
        return astVisitor.testCases();

    // check pch usage - might give false positives, but we can't do better without cost
    TestCases result;
    const QRegularExpression regex("\\bQTEST_(APPLESS_|GUILESS_)?MAIN"
                                   "\\s*\\(\\s*([[:alnum:]]+)\\s*\\)");
    QRegularExpressionMatchIterator it = regex.globalMatch(QString::fromUtf8(fileContent));
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        result.append({match.captured(2), false});
    }
    return result;
}

static CPlusPlus::Document::Ptr declaringDocument(CPlusPlus::Document::Ptr doc,
                                                  const CPlusPlus::Snapshot &snapshot,
                                                  const QString &testCaseName,
                                                  const Utils::FilePaths &alternativeFiles = {},
                                                  int *line = nullptr,
                                                  int *column = nullptr)
{
    CPlusPlus::Document::Ptr declaringDoc;
    CPlusPlus::TypeOfExpression typeOfExpr;
    typeOfExpr.init(doc, snapshot);

    QList<CPlusPlus::LookupItem> lookupItems = typeOfExpr(testCaseName.toUtf8(),
                                                          doc->globalNamespace());
    // fallback for inherited functions
    if (lookupItems.size() == 0 && !alternativeFiles.isEmpty()) {
        for (const Utils::FilePath &alternativeFile : alternativeFiles) {
            if (snapshot.contains(alternativeFile)) {
                CPlusPlus::Document::Ptr document = snapshot.document(alternativeFile);
                CPlusPlus::TypeOfExpression typeOfExpr; // we need a new one with no bindings
                typeOfExpr.init(document, snapshot);
                lookupItems = typeOfExpr(testCaseName.toUtf8(), document->globalNamespace());
                if (lookupItems.size() != 0)
                    break;
            }
        }
    }

    for (const CPlusPlus::LookupItem &item : qAsConst(lookupItems)) {
        if (CPlusPlus::Symbol *symbol = item.declaration()) {
            if (CPlusPlus::Class *toeClass = symbol->asClass()) {
                const QString declFileName = QLatin1String(toeClass->fileId()->chars(),
                                                           int(toeClass->fileId()->size()));
                declaringDoc = snapshot.document(declFileName);
                if (line)
                    *line = toeClass->line();
                if (column)
                    *column = toeClass->column() - 1;
            }
        }
    }
    return declaringDoc;
}

static QSet<Utils::FilePath> filesWithDataFunctionDefinitions(
            const QMap<QString, QtTestCodeLocationAndType> &testFunctions)
{
    QSet<Utils::FilePath> result;
    QMap<QString, QtTestCodeLocationAndType>::ConstIterator it = testFunctions.begin();
    const QMap<QString, QtTestCodeLocationAndType>::ConstIterator end = testFunctions.end();

    for ( ; it != end; ++it) {
        const QString &key = it.key();
        if (key.endsWith("_data") && testFunctions.contains(key.left(key.size() - 5)))
            result.insert(it.value().m_filePath);
    }
    return result;
}

QHash<QString, QtTestCodeLocationList> QtTestParser::checkForDataTags(
        const Utils::FilePath &fileName) const
{
    const QByteArray fileContent = getFileContent(fileName);
    CPlusPlus::Document::Ptr document = m_cppSnapshot.preprocessedDocument(fileContent, fileName);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    TestDataFunctionVisitor visitor(document);
    visitor.accept(ast);
    return visitor.dataTags();
}

/*!
 * \brief Checks whether \a testFunctions (keys are full qualified names) contains already the
 * given \a function (unqualified name).
 *
 * \return true if this function is already contained, false otherwise
 */
static bool containsFunction(const QMap<QString, QtTestCodeLocationAndType> &testFunctions,
                             const QString &function)
{
    const QString search = "::" + function;
    return Utils::anyOf(testFunctions.keys(), [&search] (const QString &key) {
        return key.endsWith(search);
    });
}

static void mergeTestFunctions(QMap<QString, QtTestCodeLocationAndType> &testFunctions,
                               const QMap<QString, QtTestCodeLocationAndType> &inheritedFunctions)
{
    static const QString dataSuffix("_data");
    // take over only inherited test functions that have not been re-implemented
    QMap<QString, QtTestCodeLocationAndType>::ConstIterator it = inheritedFunctions.begin();
    QMap<QString, QtTestCodeLocationAndType>::ConstIterator end = inheritedFunctions.end();
    for ( ; it != end; ++it) {
        const QString functionName = it.key();
        const QString &shortName = functionName.mid(functionName.lastIndexOf(':') + 1);
        if (shortName.endsWith(dataSuffix)) {
            const QString &correspondingFunc = functionName.left(functionName.size()
                                                                 - dataSuffix.size());
            // inherited test data functions only if we're inheriting the corresponding test
            // function as well (and the inherited test function is not omitted)
            if (inheritedFunctions.contains(correspondingFunc)) {
                if (!testFunctions.contains(correspondingFunc))
                    continue;
                testFunctions.insert(functionName, it.value());
            }
        } else if (!containsFunction(testFunctions, shortName)) {
            // normal test functions only if not re-implemented
            testFunctions.insert(functionName, it.value());
        }
    }
}

static void fetchAndMergeBaseTestFunctions(const QSet<QString> &baseClasses,
                                           QMap<QString, QtTestCodeLocationAndType> &testFunctions,
                                           const CPlusPlus::Document::Ptr &doc,
                                           const CPlusPlus::Snapshot &snapshot)
{
    QList<QString> bases = Utils::toList(baseClasses);
    while (!bases.empty()) {
        const QString base = bases.takeFirst();
        TestVisitor baseVisitor(base, snapshot);
        baseVisitor.setInheritedMode(true);
        CPlusPlus::Document::Ptr declaringDoc = declaringDocument(doc, snapshot, base);
        if (declaringDoc.isNull())
            continue;
        baseVisitor.accept(declaringDoc->globalNamespace());
        if (!baseVisitor.resultValid())
            continue;
        bases.append(Utils::toList(baseVisitor.baseClasses()));
        mergeTestFunctions(testFunctions, baseVisitor.privateSlots());
    }
}

static QtTestCodeLocationList tagLocationsFor(const QtTestParseResult *func,
                                              const QHash<QString, QtTestCodeLocationList> &dataTags)
{
    if (!func->inherited())
        return dataTags.value(func->name);

    QHash<QString, QtTestCodeLocationList>::ConstIterator it = dataTags.begin();
    QHash<QString, QtTestCodeLocationList>::ConstIterator end = dataTags.end();
    const int lastColon = func->name.lastIndexOf(':');
    QString funcName = lastColon == -1 ? func->name : func->name.mid(lastColon - 1);
    for ( ; it != end; ++it) {
        if (it.key().endsWith(funcName))
            return it.value();
    }
    return QtTestCodeLocationList();
}

static bool isQObject(const CPlusPlus::Document::Ptr &declaringDoc)
{
    const QString file = declaringDoc->fileName();
    return (Utils::HostOsInfo::isMacHost() && file.endsWith("QtCore.framework/Headers/qobject.h"))
            || file.endsWith("QtCore/qobject.h")  || file.endsWith("kernel/qobject.h");
}

bool QtTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                   const Utils::FilePath &fileName)
{
    CPlusPlus::Document::Ptr doc = document(fileName);
    if (doc.isNull())
        return false;
    const TestCases &oldTestCases = m_testCases.value(fileName);
    if ((!includesQtTest(doc, m_cppSnapshot) || !qtTestLibDefined(fileName))
        && oldTestCases.isEmpty()) {
        return false;
    }

    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    TestCases testCaseList(testCases(modelManager, fileName));
    bool reported = false;
    // we might be in a reparse without the original entry point with the QTest::qExec()
    if (testCaseList.isEmpty() && !oldTestCases.empty())
        testCaseList.append(oldTestCases);
    for (const TestCase &testCase : testCaseList) {
        if (!testCase.name.isEmpty()) {
            TestCaseData data;
            Utils::optional<bool> earlyReturn = fillTestCaseData(testCase.name, doc, data);
            if (earlyReturn.has_value() || !data.valid)
                continue;

            QList<CppTools::ProjectPart::Ptr> projectParts = modelManager->projectPart(fileName);
            if (projectParts.isEmpty()) // happens if shutting down while parsing
                return false;

            data.multipleTestCases = testCase.multipleTestCases;
            QtTestParseResult *parseResult
                    = createParseResult(testCase.name, data, projectParts.first()->projectFile);
            futureInterface.reportResult(TestParseResultPtr(parseResult));
            reported = true;
        }
    }
    return reported;
}

Utils::optional<bool> QtTestParser::fillTestCaseData(
        const QString &testCaseName, const CPlusPlus::Document::Ptr &doc,
        TestCaseData &data) const
{
    const Utils::FilePath filePath = Utils::FilePath::fromString(doc->fileName());
    const Utils::FilePaths &alternativeFiles = m_alternativeFiles.values(filePath);
    CPlusPlus::Document::Ptr declaringDoc = declaringDocument(doc, m_cppSnapshot, testCaseName,
                                                              alternativeFiles,
                                                              &(data.line), &(data.column));
    if (declaringDoc.isNull())
        return false;

    TestVisitor visitor(testCaseName, m_cppSnapshot);
    visitor.accept(declaringDoc->globalNamespace());
    if (!visitor.resultValid())
        return false;

    data.testFunctions = visitor.privateSlots();
    // gather appropriate information of base classes as well and merge into already found
    // functions - but only as far as QtTest can handle this appropriate
    fetchAndMergeBaseTestFunctions(
                visitor.baseClasses(), data.testFunctions, declaringDoc, m_cppSnapshot);

    // handle tests that are not runnable without more information (plugin unit test of QC)
    if (data.testFunctions.isEmpty() && testCaseName == "QObject" && isQObject(declaringDoc))
        return true; // we did not handle it, but we do not expect any test defined there either

    const QSet<Utils::FilePath> &files = filesWithDataFunctionDefinitions(data.testFunctions);
    for (const Utils::FilePath &file : files)
        Utils::addToHash(&(data.dataTags), checkForDataTags(file));

    data.fileName = Utils::FilePath::fromString(declaringDoc->fileName());
    data.valid = true;
    return Utils::optional<bool>();
}

QtTestParseResult *QtTestParser::createParseResult(
        const QString &testCaseName, const TestCaseData &data, const QString &projectFile) const
{
    QtTestParseResult *parseResult = new QtTestParseResult(framework());
    parseResult->itemType = TestTreeItem::TestCase;
    parseResult->fileName = data.fileName;
    parseResult->name = testCaseName;
    parseResult->displayName = testCaseName;
    parseResult->line = data.line;
    parseResult->column = data.column;
    parseResult->proFile = Utils::FilePath::fromString(projectFile);
    parseResult->setRunsMultipleTestcases(data.multipleTestCases);
    QMap<QString, QtTestCodeLocationAndType>::ConstIterator it = data.testFunctions.begin();
    const QMap<QString, QtTestCodeLocationAndType>::ConstIterator end = data.testFunctions.end();

    for ( ; it != end; ++it) {
        const QtTestCodeLocationAndType &location = it.value();
        QtTestParseResult *func = new QtTestParseResult(framework());
        func->itemType = location.m_type;
        func->name = location.m_name;
        func->displayName = location.m_name.mid(location.m_name.lastIndexOf(':') + 1);
        func->fileName = location.m_filePath;
        func->line = location.m_line;
        func->column = location.m_column;
        func->setInherited(location.m_inherited);
        func->setRunsMultipleTestcases(data.multipleTestCases);

        const QtTestCodeLocationList &tagLocations = tagLocationsFor(func, data.dataTags);
        for (const QtTestCodeLocationAndType &tag : tagLocations) {
            QtTestParseResult *dataTag = new QtTestParseResult(framework());
            dataTag->itemType = tag.m_type;
            dataTag->name = tag.m_name;
            dataTag->displayName = tag.m_name;
            dataTag->fileName = Utils::FilePath::fromString(
                        data.testFunctions.value(it.key() + "_data").m_name);
            dataTag->line = tag.m_line;
            dataTag->column = tag.m_column;
            dataTag->setInherited(tag.m_inherited);
            dataTag->setRunsMultipleTestcases(data.multipleTestCases);

            func->children.append(dataTag);
        }
        parseResult->children.append(func);
    }
    return parseResult;
}

void QtTestParser::init(const Utils::FilePaths &filesToParse, bool fullParse)
{
    if (!fullParse) { // in a full parse cached information might lead to wrong results
        m_testCases = QTestUtils::testCaseNamesForFiles(framework(), filesToParse);
        m_alternativeFiles = QTestUtils::alternativeFiles(framework(), filesToParse);
    }
    CppParser::init(filesToParse, fullParse);
}

void QtTestParser::release()
{
    m_testCases.clear();
    m_alternativeFiles.clear();
    CppParser::release();
}

} // namespace Internal
} // namespace Autotest
