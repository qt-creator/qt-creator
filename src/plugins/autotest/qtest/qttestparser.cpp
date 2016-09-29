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

namespace Autotest {
namespace Internal {

TestTreeItem *QtTestParseResult::createTestTreeItem() const
{
    return itemType == TestTreeItem::Root ? 0 : QtTestTreeItem::createTestItem(this);
}

static bool includesQtTest(const CPlusPlus::Document::Ptr &doc, const CPlusPlus::Snapshot &snapshot)
{
    static QStringList expectedHeaderPrefixes
            = Utils::HostOsInfo::isMacHost()
            ? QStringList({ "QtTest.framework/Headers", "QtTest" }) : QStringList({ "QtTest" });

    const QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    foreach (const CPlusPlus::Document::Include &inc, includes) {
        // TODO this short cut works only for #include <QtTest>
        // bad, as there could be much more different approaches
        if (inc.unresolvedFileName() == QLatin1String("QtTest")) {
            foreach (const QString &prefix, expectedHeaderPrefixes) {
                if (inc.resolvedFileName().endsWith(QString("%1/QtTest").arg(prefix)))
                    return true;
            }
        }
    }

    const QSet<QString> allIncludes = snapshot.allIncludesForDocument(doc->fileName());
    foreach (const QString &include, allIncludes) {
        foreach (const QString &prefix, expectedHeaderPrefixes) {
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

    foreach (const CPlusPlus::Document::MacroUse &macro, macros) {
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
                                                  unsigned *line, unsigned *column)
{
    CPlusPlus::Document::Ptr declaringDoc = doc;
    CPlusPlus::TypeOfExpression typeOfExpr;
    typeOfExpr.init(doc, snapshot);

    QList<CPlusPlus::LookupItem> lookupItems = typeOfExpr(testCaseName.toUtf8(),
                                                          doc->globalNamespace());
    if (lookupItems.size()) {
        if (CPlusPlus::Symbol *symbol = lookupItems.first().declaration()) {
            if (CPlusPlus::Class *toeClass = symbol->asClass()) {
                const QString declFileName = QLatin1String(toeClass->fileId()->chars(),
                                                           toeClass->fileId()->size());
                declaringDoc = snapshot.document(declFileName);
                *line = toeClass->line();
                *column = toeClass->column() - 1;
            }
        }
    }
    return declaringDoc;
}

static QSet<QString> filesWithDataFunctionDefinitions(
            const QMap<QString, TestCodeLocationAndType> &testFunctions)
{
    QSet<QString> result;
    QMap<QString, TestCodeLocationAndType>::ConstIterator it = testFunctions.begin();
    const QMap<QString, TestCodeLocationAndType>::ConstIterator end = testFunctions.end();

    for ( ; it != end; ++it) {
        const QString &key = it.key();
        if (key.endsWith("_data") && testFunctions.contains(key.left(key.size() - 5)))
            result.insert(it.value().m_name);
    }
    return result;
}

static QMap<QString, TestCodeLocationList> checkForDataTags(const QString &fileName,
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

static bool handleQtTest(QFutureInterface<TestParseResultPtr> futureInterface,
                         CPlusPlus::Document::Ptr document,
                         const CPlusPlus::Snapshot &snapshot,
                         const QString &oldTestCaseName,
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
                                                                  &line, &column);
        if (declaringDoc.isNull())
            return false;

        TestVisitor visitor(testCaseName, snapshot);
        visitor.accept(declaringDoc->globalNamespace());
        if (!visitor.resultValid())
            return false;

        const QMap<QString, TestCodeLocationAndType> &testFunctions = visitor.privateSlots();
        const QSet<QString> &files = filesWithDataFunctionDefinitions(testFunctions);

        // TODO: change to QHash<>
        QMap<QString, TestCodeLocationList> dataTags;
        foreach (const QString &file, files)
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
        QMap<QString, TestCodeLocationAndType>::ConstIterator it = testFunctions.begin();
        const QMap<QString, TestCodeLocationAndType>::ConstIterator end = testFunctions.end();
        for ( ; it != end; ++it) {
            const TestCodeLocationAndType &location = it.value();
            QtTestParseResult *func = new QtTestParseResult(id);
            func->itemType = location.m_type;
            func->name = testCaseName + "::" + it.key();
            func->displayName = it.key();
            func->fileName = location.m_name;
            func->line = location.m_line;
            func->column = location.m_column;

            foreach (const TestCodeLocationAndType &tag, dataTags.value(func->name)) {
                QtTestParseResult *dataTag = new QtTestParseResult(id);
                dataTag->itemType = tag.m_type;
                dataTag->name = tag.m_name;
                dataTag->displayName = tag.m_name;
                dataTag->fileName = testFunctions.value(it.key() + "_data").m_name;
                dataTag->line = tag.m_line;
                dataTag->column = tag.m_column;

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
    CppParser::init(filesToParse);
}

void QtTestParser::release()
{
    m_testCaseNames.clear();
    CppParser::release();
}

bool QtTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                   const QString &fileName)
{
    if (!m_cppSnapshot.contains(fileName) || !selectedForBuilding(fileName))
        return false;
    CPlusPlus::Document::Ptr doc = m_cppSnapshot.find(fileName).value();
    const QString &oldName = m_testCaseNames.value(fileName);
    if ((!includesQtTest(doc, m_cppSnapshot) || !qtTestLibDefined(fileName)) && oldName.isEmpty())
        return false;

    return handleQtTest(futureInterface, doc, m_cppSnapshot, oldName, id());
}

} // namespace Internal
} // namespace Autotest
