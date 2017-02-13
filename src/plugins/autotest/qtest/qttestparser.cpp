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
#include "qttesttreeitem.h"
#include "qttestvisitors.h"
#include "qttest_utils.h"
#include "../autotest_utils.h"

#include <cplusplus/TypeOfExpression.h>
#include <utils/algorithm.h>

namespace Autotest {
namespace Internal {

TestTreeItem *QtTestParseResult::createTestTreeItem() const
{
    if (itemType == TestTreeItem::Root)
        return nullptr;

    QtTestTreeItem *item = new QtTestTreeItem(displayName, fileName, itemType);
    item->setProFile(proFile);
    item->setLine(line);
    item->setColumn(column);
    item->setInherited(m_inherited);

    for (const TestParseResult *funcParseResult : children)
        item->appendChild(funcParseResult->createTestTreeItem());
    return item;
}

static bool includesQtTest(const CPlusPlus::Document::Ptr &doc, const CPlusPlus::Snapshot &snapshot)
{
    static QStringList expectedHeaderPrefixes
            = Utils::HostOsInfo::isMacHost()
            ? QStringList({ "QtTest.framework/Headers", "QtTest" }) : QStringList({ "QtTest" });

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
    return false;
}

static bool qtTestLibDefined(const QString &fileName)
{
    const QList<CppTools::ProjectPart::Ptr> parts =
            CppTools::CppModelManager::instance()->projectPart(fileName);
    if (parts.size() > 0)
        return parts.at(0)->projectDefines.contains("#define QT_TESTLIB_LIB");
    return false;
}

static QString testClass(const CppTools::CppModelManager *modelManager,
                         const CPlusPlus::Snapshot &snapshot, const QString &fileName)
{
    const QByteArray &fileContent = CppParser::getFileContent(fileName);
    CPlusPlus::Document::Ptr document = modelManager->document(fileName);
    if (document.isNull())
        return QString();

    const QList<CPlusPlus::Document::MacroUse> macros = document->macroUses();

    for (const CPlusPlus::Document::MacroUse &macro : macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (QTestUtils::isQTestMacro(name)) {
            const CPlusPlus::Document::Block arg = macro.arguments().at(0);
            return QLatin1String(fileContent.mid(arg.bytesBegin(),
                                                 arg.bytesEnd() - arg.bytesBegin()));
        }
    }
    // check if one has used a self-defined macro or QTest::qExec() directly
    document = snapshot.preprocessedDocument(fileContent, fileName);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    TestAstVisitor astVisitor(document, snapshot);
    astVisitor.accept(ast);
    return astVisitor.className();
}

static CPlusPlus::Document::Ptr declaringDocument(CPlusPlus::Document::Ptr doc,
                                                  const CPlusPlus::Snapshot &snapshot,
                                                  const QString &testCaseName,
                                                  const QStringList &alternativeFiles = {},
                                                  unsigned *line = 0, unsigned *column = 0)
{
    CPlusPlus::Document::Ptr declaringDoc;
    CPlusPlus::TypeOfExpression typeOfExpr;
    typeOfExpr.init(doc, snapshot);

    QList<CPlusPlus::LookupItem> lookupItems = typeOfExpr(testCaseName.toUtf8(),
                                                          doc->globalNamespace());
    // fallback for inherited functions
    if (lookupItems.size() == 0 && !alternativeFiles.isEmpty()) {
        for (const QString &alternativeFile : alternativeFiles) {
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

    if (lookupItems.size()) {
        if (CPlusPlus::Symbol *symbol = lookupItems.first().declaration()) {
            if (CPlusPlus::Class *toeClass = symbol->asClass()) {
                const QString declFileName = QLatin1String(toeClass->fileId()->chars(),
                                                           toeClass->fileId()->size());
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

static QSet<QString> filesWithDataFunctionDefinitions(
            const QMap<QString, QtTestCodeLocationAndType> &testFunctions)
{
    QSet<QString> result;
    QMap<QString, QtTestCodeLocationAndType>::ConstIterator it = testFunctions.begin();
    const QMap<QString, QtTestCodeLocationAndType>::ConstIterator end = testFunctions.end();

    for ( ; it != end; ++it) {
        const QString &key = it.key();
        if (key.endsWith("_data") && testFunctions.contains(key.left(key.size() - 5)))
            result.insert(it.value().m_name);
    }
    return result;
}

static QMap<QString, QtTestCodeLocationList> checkForDataTags(const QString &fileName,
            const CPlusPlus::Snapshot &snapshot)
{
    const QByteArray fileContent = CppParser::getFileContent(fileName);
    CPlusPlus::Document::Ptr document = snapshot.preprocessedDocument(fileContent, fileName);
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
    QList<QString> bases = baseClasses.toList();
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
        bases.append(baseVisitor.baseClasses().toList());
        mergeTestFunctions(testFunctions, baseVisitor.privateSlots());
    }
}

static QtTestCodeLocationList tagLocationsFor(const QtTestParseResult *func,
                                              const QMap<QString, QtTestCodeLocationList> &dataTags)
{
    if (!func->inherited())
        return dataTags.value(func->name);

    QMap<QString, QtTestCodeLocationList>::ConstIterator it = dataTags.begin();
    QMap<QString, QtTestCodeLocationList>::ConstIterator end = dataTags.end();
    const int lastColon = func->name.lastIndexOf(':');
    QString funcName = lastColon == -1 ? func->name : func->name.mid(lastColon - 1);
    for ( ; it != end; ++it) {
        if (it.key().endsWith(funcName))
            return it.value();
    }
    return QtTestCodeLocationList();
}

static bool handleQtTest(QFutureInterface<TestParseResultPtr> futureInterface,
                         CPlusPlus::Document::Ptr document,
                         const CPlusPlus::Snapshot &snapshot,
                         const QString &oldTestCaseName,
                         const QStringList &alternativeFiles,
                         const Core::Id &id)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QString &fileName = document->fileName();
    QString testCaseName(testClass(modelManager, snapshot, fileName));
    // we might be in a reparse without the original entry point with the QTest::qExec()
    if (testCaseName.isEmpty())
        testCaseName = oldTestCaseName;
    if (!testCaseName.isEmpty()) {
        unsigned line = 0;
        unsigned column = 0;
        CPlusPlus::Document::Ptr declaringDoc = declaringDocument(document, snapshot, testCaseName,
                                                                  alternativeFiles, &line, &column);
        if (declaringDoc.isNull())
            return false;

        TestVisitor visitor(testCaseName, snapshot);
        visitor.accept(declaringDoc->globalNamespace());
        if (!visitor.resultValid())
            return false;

        QMap<QString, QtTestCodeLocationAndType> testFunctions = visitor.privateSlots();
        // gather appropriate information of base classes as well and merge into already found
        // functions - but only as far as QtTest can handle this appropriate
        fetchAndMergeBaseTestFunctions(
                    visitor.baseClasses(), testFunctions, declaringDoc, snapshot);

        const QSet<QString> &files = filesWithDataFunctionDefinitions(testFunctions);

        // TODO: change to QHash<>
        QMap<QString, QtTestCodeLocationList> dataTags;
        for (const QString &file : files)
            dataTags.unite(checkForDataTags(file, snapshot));

        QtTestParseResult *parseResult = new QtTestParseResult(id);
        parseResult->itemType = TestTreeItem::TestCase;
        parseResult->fileName = declaringDoc->fileName();
        parseResult->name = testCaseName;
        parseResult->displayName = testCaseName;
        parseResult->line = line;
        parseResult->column = column;
        QList<CppTools::ProjectPart::Ptr> projectParts = modelManager->projectPart(fileName);
        if (projectParts.isEmpty()) // happens if shutting down while parsing
            return false;
        parseResult->proFile = projectParts.first()->projectFile;
        QMap<QString, QtTestCodeLocationAndType>::ConstIterator it = testFunctions.begin();
        const QMap<QString, QtTestCodeLocationAndType>::ConstIterator end = testFunctions.end();
        for ( ; it != end; ++it) {
            const QtTestCodeLocationAndType &location = it.value();
            QString functionName = it.key();
            functionName = functionName.mid(functionName.lastIndexOf(':') + 1);
            QtTestParseResult *func = new QtTestParseResult(id);
            func->itemType = location.m_type;
            func->name = testCaseName + "::" + functionName;
            func->displayName = functionName;
            func->fileName = location.m_name;
            func->line = location.m_line;
            func->column = location.m_column;
            func->setInherited(location.m_inherited);

            const QtTestCodeLocationList &tagLocations = tagLocationsFor(func, dataTags);
            for (const QtTestCodeLocationAndType &tag : tagLocations) {
                QtTestParseResult *dataTag = new QtTestParseResult(id);
                dataTag->itemType = tag.m_type;
                dataTag->name = tag.m_name;
                dataTag->displayName = tag.m_name;
                dataTag->fileName = testFunctions.value(it.key() + "_data").m_name;
                dataTag->line = tag.m_line;
                dataTag->column = tag.m_column;
                dataTag->setInherited(tag.m_inherited);

                func->children.append(dataTag);
            }
            parseResult->children.append(func);
        }

        futureInterface.reportResult(TestParseResultPtr(parseResult));
        return true;
    }
    return false;
}

void QtTestParser::init(const QStringList &filesToParse)
{
    m_testCaseNames = QTestUtils::testCaseNamesForFiles(id(), filesToParse);
    m_alternativeFiles = QTestUtils::alternativeFiles(id(), filesToParse);
    CppParser::init(filesToParse);
}

void QtTestParser::release()
{
    m_testCaseNames.clear();
    m_alternativeFiles.clear();
    CppParser::release();
}

bool QtTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                   const QString &fileName)
{
    if (!m_cppSnapshot.contains(fileName) || !selectedForBuilding(fileName))
        return false;
    CPlusPlus::Document::Ptr doc = m_cppSnapshot.find(fileName).value();
    const QString &oldName = m_testCaseNames.value(fileName);
    const QStringList &alternativeFiles = m_alternativeFiles.values(fileName);
    if ((!includesQtTest(doc, m_cppSnapshot) || !qtTestLibDefined(fileName)) && oldName.isEmpty())
        return false;

    return handleQtTest(futureInterface, doc, m_cppSnapshot, oldName, alternativeFiles, id());
}

} // namespace Internal
} // namespace Autotest
