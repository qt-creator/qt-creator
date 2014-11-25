/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "autotestconstants.h"
#include "testcodeparser.h"
#include "testinfo.h"
#include "testtreeitem.h"
#include "testtreemodel.h"
#include "testvisitor.h"

#include <cplusplus/LookupContext.h>
#include <cplusplus/TypeOfExpression.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppworkingcopy.h>

#include <projectexplorer/session.h>

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdialect.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <utils/textfileformat.h>

namespace Autotest {
namespace Internal {

TestCodeParser::TestCodeParser(TestTreeModel *parent)
    : QObject(parent),
      m_model(parent),
      m_currentProject(0)
{
}

TestCodeParser::~TestCodeParser()
{
    clearMaps();
}

void TestCodeParser::updateTestTree()
{
    qDebug("updating TestTreeModel");

    clearMaps();
    m_model->removeAllAutoTests();
    m_model->removeAllQuickTests();
    const ProjectExplorer::SessionManager *session = ProjectExplorer::SessionManager::instance();
    if (!session || !session->hasProjects()) {
        if (m_currentProject) {
            if (QmakeProjectManager::QmakeProject *qmproj
                    = qobject_cast<QmakeProjectManager::QmakeProject *>(m_currentProject)) {
                disconnect(qmproj, &QmakeProjectManager::QmakeProject::proFilesEvaluated,
                           this, &TestCodeParser::onProFileEvaluated);
            }
        }
        m_currentProject = 0;
        return;
    }

    m_currentProject = session->startupProject();
    if (!m_currentProject)
        return;
    else {
        if (QmakeProjectManager::QmakeProject *qmproj
                = qobject_cast<QmakeProjectManager::QmakeProject *>(m_currentProject)) {
            connect(qmproj, &QmakeProjectManager::QmakeProject::proFilesEvaluated,
                    this, &TestCodeParser::onProFileEvaluated);
        }
    }
    scanForTests();
}

/****** scan for QTest related stuff helpers ******/

static QByteArray getFileContent(QString filePath)
{
    QByteArray fileContent;
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    CppTools::WorkingCopy wc = cppMM->workingCopy();
    if (wc.contains(filePath)) {
        fileContent = wc.source(filePath);
    } else {
        QString error;
        const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
        if (Utils::TextFileFormat::readFileUTF8(filePath, codec, &fileContent, &error)
                != Utils::TextFileFormat::ReadSuccess) {
            qDebug() << "Failed to read file" << filePath << ":" << error;
        }
    }
    return fileContent;
}

static bool includesQtTest(const CPlusPlus::Document::Ptr &doc,
                           const CppTools::CppModelManager *cppMM)
{
    const QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    foreach (const CPlusPlus::Document::Include &inc, includes) {
        // TODO this short cut works only for #include <QtTest>
        // bad, as there could be much more different approaches
        if (inc.unresolvedFileName() == QLatin1String("QtTest")
                && inc.resolvedFileName().endsWith(QLatin1String("QtTest/QtTest"))) {
            return true;
        }
    }

    if (cppMM) {
        CPlusPlus::Snapshot snapshot = cppMM->snapshot();
        const QSet<QString> allIncludes = snapshot.allIncludesForDocument(doc->fileName());
        foreach (const QString &include, allIncludes) {
            if (include.endsWith(QLatin1String("QtTest/qtest.h"))) {
                return true;
            }
        }
    }
    return false;
}

static bool includesQtQuickTest(const CPlusPlus::Document::Ptr &doc,
                                const CppTools::CppModelManager *cppMM)
{
    const QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    foreach (const CPlusPlus::Document::Include &inc, includes) {
        if (inc.unresolvedFileName() == QLatin1String("QtQuickTest/quicktest.h")
                && inc.resolvedFileName().endsWith(QLatin1String("QtQuickTest/quicktest.h"))) {
            return true;
        }
    }

    if (cppMM) {
        foreach (const QString &include, cppMM->snapshot().allIncludesForDocument(doc->fileName())) {
            if (include.endsWith(QLatin1String("QtQuickTest/quicktest.h")))
                return true;
        }
    }
    return false;
}

static bool qtTestLibDefined(const CppTools::CppModelManager *cppMM,
                             const QString &fileName)
{
    const QList<CppTools::ProjectPart::Ptr> parts = cppMM->projectPart(fileName);
    if (parts.size() > 0)
        return parts.at(0)->projectDefines.contains("#define QT_TESTLIB_LIB 1");
    return false;
}

static QString quickTestSrcDir(const CppTools::CppModelManager *cppMM,
                               const QString &fileName)
{
    static const QByteArray qtsd(" QUICK_TEST_SOURCE_DIR ");
    const QList<CppTools::ProjectPart::Ptr> parts = cppMM->projectPart(fileName);
    if (parts.size() > 0) {
        QByteArray projDefines(parts.at(0)->projectDefines);
        foreach (const QByteArray &line, projDefines.split('\n')) {
            if (line.contains(qtsd)) {
                QByteArray result = line.mid(line.indexOf(qtsd) + qtsd.length());
                if (result.startsWith('"'))
                    result.remove(result.length() - 1, 1).remove(0, 1);
                if (result.startsWith("\\\""))
                    result.remove(result.length() - 2, 2).remove(0, 2);
                return QLatin1String(result);
            }
        }
    }
    return QString();
}

static QString testClass(const CPlusPlus::Document::Ptr &doc)
{
    static const QByteArray qtTestMacros[] = {"QTEST_MAIN", "QTEST_APPLESS_MAIN", "QTEST_GUILESS_MAIN"};
    const QList<CPlusPlus::Document::MacroUse> macros = doc->macroUses();

    foreach (const CPlusPlus::Document::MacroUse &macro, macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (name == qtTestMacros[0] || name == qtTestMacros[1] || name == qtTestMacros[2]) {
            const CPlusPlus::Document::Block arg = macro.arguments().at(0);
            return QLatin1String(getFileContent(doc->fileName()).mid(arg.bytesBegin(), arg.bytesEnd() - arg.bytesBegin()));
        }
    }
    return QString();
}

static QString quickTestName(const CPlusPlus::Document::Ptr &doc)
{
    static const QByteArray qtTestMacros[] = {"QUICK_TEST_MAIN", "QUICK_TEST_OPENGL_MAIN"};
    const QList<CPlusPlus::Document::MacroUse> macros = doc->macroUses();

    foreach (const CPlusPlus::Document::MacroUse &macro, macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (name == qtTestMacros[0] || name == qtTestMacros[1] || name == qtTestMacros[2]) {
            CPlusPlus::Document::Block arg = macro.arguments().at(0);
            return QLatin1String(getFileContent(doc->fileName()).mid(arg.bytesBegin(), arg.bytesEnd() - arg.bytesBegin()));
        }
    }
    return QString();
}

static QList<QmlJS::Document::Ptr> scanDirectoryForQuickTestQmlFiles(const QString &srcDir)
{
    QStringList dirs(srcDir);
    QmlJS::ModelManagerInterface *qmlJsMM = QmlJSTools::Internal::ModelManager::instance();
    // make sure even files not listed in pro file are available inside the snapshot
    QFutureInterface<void> future;
    QmlJS::PathsAndLanguages paths;
    paths.maybeInsert(Utils::FileName::fromString(srcDir), QmlJS::Dialect::Qml);
    QmlJS::ModelManagerInterface::importScan(future, qmlJsMM->workingCopy(),
                                             paths, qmlJsMM, false, false);

    const QmlJS::Snapshot snapshot = QmlJSTools::Internal::ModelManager::instance()->snapshot();
    QDirIterator it(srcDir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi(it.fileInfo().canonicalFilePath());
        dirs << fi.filePath();
    }
    QList<QmlJS::Document::Ptr> foundDocs;

    foreach (const QString &path, dirs) {
        const QList<QmlJS::Document::Ptr> docs = snapshot.documentsInDirectory(path);
        foreach (const QmlJS::Document::Ptr &doc, docs) {
            const QString fileName(QFileInfo(doc->fileName()).fileName());
            if (fileName.startsWith(QLatin1String("tst_")) && fileName.endsWith(QLatin1String(".qml")))
                foundDocs << doc;
        }
    }

    return foundDocs;
}

/****** end of helpers ******/

void TestCodeParser::checkDocumentForTestCode(CPlusPlus::Document::Ptr doc)
{
    const QString file = doc->fileName();
    const CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();

    QList<CppTools::ProjectPart::Ptr> projParts = cppMM->projectPart(file);
    if (projParts.size())
        if (!projParts.at(0)->selectedForBuilding) {
            removeTestsIfNecessary(file);
            return;
        }

    if (includesQtQuickTest(doc, cppMM)) {
        handleQtQuickTest(doc);
        return;
    }

    if (includesQtTest(doc, cppMM) && qtTestLibDefined(cppMM, file)) {
        QString tc(testClass(doc));
        if (tc.isEmpty()) {
            // one might have used an approach without macros or defined own macros
            const CPlusPlus::Snapshot snapshot = cppMM->snapshot();
            const QByteArray fileContent = getFileContent(file);
            doc = snapshot.preprocessedDocument(fileContent, file);
            doc->check();
            CPlusPlus::AST *ast = doc->translationUnit()->ast();
            TestAstVisitor astVisitor(doc);
            astVisitor.accept(ast);
            tc = astVisitor.className();
        }
        if (!tc.isEmpty()) {
            // construct the new/modified TestTreeItem
            const QModelIndex autoTestRootIndex = m_model->index(0, 0);
            TestTreeItem *autoTestRootItem = static_cast<TestTreeItem *>(autoTestRootIndex.internalPointer());
            TestTreeItem *ttItem = new TestTreeItem(tc, file, TestTreeItem::TEST_CLASS,
                                                    autoTestRootItem);
            QString declFileName;
            CPlusPlus::TypeOfExpression toe;
            toe.init(doc, cppMM->snapshot());
            CPlusPlus::Document::Ptr declaringDoc = doc;
            const QList<CPlusPlus::LookupItem>  toeItems = toe(tc.toUtf8(), doc->globalNamespace());
            if (toeItems.size()) {
                CPlusPlus::Class *toeClass = toeItems.first().declaration()->asClass();
                if (toeClass) {
                    declFileName = QLatin1String(toeClass->fileId()->chars(),
                                                 toeClass->fileId()->size());
                    declaringDoc = cppMM->snapshot().document(declFileName);
                    ttItem->setFilePath(declFileName);
                    ttItem->setLine(toeClass->line());
                    ttItem->setColumn(toeClass->column() - 1);
                }
            }
            if (declaringDoc.isNull()) {
                delete ttItem;
                return;
            }
            TestVisitor myVisitor(tc);
            myVisitor.accept(declaringDoc->globalNamespace());
            const QMap<QString, TestCodeLocationAndType> privSlots = myVisitor.privateSlots();
            foreach (const QString &privS, privSlots.keys()) {
                const TestCodeLocationAndType locationAndType = privSlots.value(privS);
                TestTreeItem *ttSub = new TestTreeItem(privS, locationAndType.m_fileName,
                                                       locationAndType.m_type, ttItem);
                ttSub->setLine(locationAndType.m_line);
                ttSub->setColumn(locationAndType.m_column);
                ttItem->appendChild(ttSub);
            }

            // TODO refactoring?
            // update model and internal map
            QString proFile;
            QList<CppTools::ProjectPart::Ptr> ppList = cppMM->projectPart(file);
            if (ppList.size())
                proFile = ppList.at(0)->projectFile;

            TestInfo info;
            int count;
            if (m_cppDocMap.contains(file)) {
                info = m_cppDocMap.value(file);
                count = autoTestRootItem->childCount();
                for (int i = 0; i < count; ++i) {
                    TestTreeItem *currentItem = autoTestRootItem->child(i);
                    if (currentItem->filePath() == file) {
                        m_model->modifyAutoTestSubtree(i, ttItem);
                        TestInfo ti(tc, privSlots.keys(), doc->revision(), doc->editorRevision());
                        ti.setProfile(proFile);
                        m_cppDocMap.insert(file, ti);
                        break;
                    }
                }
                if (declFileName != file) {
                    info = m_cppDocMap.value(declFileName);
                    count = autoTestRootItem->childCount();
                    for (int i = 0; i < count; ++i) {
                        TestTreeItem *currentItem = autoTestRootItem->child(i);
                        if (currentItem->filePath() == declFileName) {
                            m_model->modifyAutoTestSubtree(i, ttItem);
                            TestInfo ti(tc, privSlots.keys(), doc->revision(),
                                        doc->editorRevision());
                            ti.setReferencingFile(file);
                            ti.setProfile(proFile);
                            m_cppDocMap.insert(declFileName, ti);
                            break;
                        }
                    }
                }
                delete ttItem;
            } else {
                m_model->addAutoTest(ttItem);
                TestInfo ti(tc, privSlots.keys(), doc->revision(), doc->editorRevision());
                ti.setProfile(proFile);
                m_cppDocMap.insert(file, ti);
                if (declFileName != file) {
                    TestInfo ti(tc, privSlots.keys(), doc->revision(), doc->editorRevision());
                    ti.setReferencingFile(file);
                    ti.setProfile(proFile);
                    m_cppDocMap.insert(declFileName, ti);
                }
            }
        }
    } else {
        // could not find the class to test, but QTest is included and QT_TESTLIB_LIB defined
        // maybe file is only a referenced file
        if (m_cppDocMap.contains(file)) {
            const TestInfo info = m_cppDocMap[file];
            CPlusPlus::Snapshot snapshot = cppMM->snapshot();
            if (snapshot.contains(info.referencingFile())) {
                checkDocumentForTestCode(snapshot.find(info.referencingFile()).value());
            }
        }
    }
}

void TestCodeParser::handleQtQuickTest(CPlusPlus::Document::Ptr doc)
{
    const CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();

    if (quickTestName(doc).isEmpty())
        return;

    const QString srcDir = quickTestSrcDir(cppMM, doc->fileName());
    if (srcDir.isEmpty())
        return;

    const QList<QmlJS::Document::Ptr> qmlDocs = scanDirectoryForQuickTestQmlFiles(srcDir);
    foreach (const QmlJS::Document::Ptr &d, qmlDocs) {
        QmlJS::AST::Node *ast = d->ast();
        if (!ast) {
            qDebug() << "ast is zero pointer" << d->fileName(); // should not happen
            continue;
        }
        TestQmlVisitor qmlVisitor(d);
        QmlJS::AST::Node::accept(ast, &qmlVisitor);

        const QString tcName = qmlVisitor.testCaseName();
        const TestCodeLocationAndType tcLocationAndType = qmlVisitor.testCaseLocation();
        const QMap<QString, TestCodeLocationAndType> testFunctions = qmlVisitor.testFunctions();

        const QModelIndex quickTestRootIndex = m_model->index(1, 0);
        TestTreeItem *quickTestRootItem = static_cast<TestTreeItem *>(quickTestRootIndex.internalPointer());

        if (tcName.isEmpty()) {
            // if this test case was named before remove it
            if (m_quickDocMap.contains(d->fileName())) {
                m_model->removeQuickTestSubtreeByFilePath(d->fileName());
                m_quickDocMap.remove(d->fileName());
            }
            TestTreeItem *unnamedQTItem = m_model->unnamedQuickTests();
            if (unnamedQTItem) {
                // remove unnamed quick tests that are already found for this qml file
                if (m_model->removeUnnamedQuickTests(d->fileName())) {
                    // make sure m_quickDocMap does not have a inconsistent state now
                    TestInfo ti = m_quickDocMap[tr(Constants::UNNAMED_QUICKTESTS)];
                    QStringList tiFunctions = ti.testFunctions();
                    foreach (const QString &func, testFunctions.keys())
                        tiFunctions.removeOne(func);
                    ti.setTestFunctions(tiFunctions);
                    if (tiFunctions.size() == 0)
                        m_quickDocMap.remove(tr(Constants::UNNAMED_QUICKTESTS));
                    else
                        m_quickDocMap.insert(tr(Constants::UNNAMED_QUICKTESTS), ti);
                }
                // as removeUnnamedQuickTests() could delete this item itself try to get it again
                unnamedQTItem = m_model->unnamedQuickTests();
            }
            // construct new/modified TestTreeItem
            TestTreeItem *ttItem = new TestTreeItem(QString(), QString(),
                                                    TestTreeItem::TEST_CLASS,
                                                    quickTestRootItem);
            if (unnamedQTItem) {
                for (int i = 0, count = unnamedQTItem->childCount(); i < count; ++i) {
                    TestTreeItem *child = new TestTreeItem(*unnamedQTItem->child(i));
                    child->setParent(ttItem);
                    ttItem->appendChild(child);
                }
            }

            foreach (const QString &func, testFunctions.keys()) {
                const TestCodeLocationAndType locationAndType = testFunctions.value(func);
                TestTreeItem *ttSub = new TestTreeItem(func, locationAndType.m_fileName,
                                                       locationAndType.m_type, ttItem);
                ttSub->setLine(locationAndType.m_line);
                ttSub->setColumn(locationAndType.m_column);
                ttSub->setMainFile(doc->fileName());
                ttItem->appendChild(ttSub);
            }
            TestInfo info = m_quickDocMap.contains(tr(Constants::UNNAMED_QUICKTESTS))
                    ? m_quickDocMap[tr(Constants::UNNAMED_QUICKTESTS)]
                    : TestInfo(QString(), QStringList(), 666);
            QStringList originalFunctions(info.testFunctions());
            foreach (const QString &func, testFunctions.keys())
                originalFunctions.append(func);
            info.setTestFunctions(originalFunctions);

            if (unnamedQTItem) {
                m_model->modifyQuickTestSubtree(unnamedQTItem->row(), ttItem);
                delete ttItem;
            } else {
                m_model->addQuickTest(ttItem);
            }
            m_quickDocMap.insert(tr(Constants::UNNAMED_QUICKTESTS), info);

            continue;
        } // end of handling test cases without name property

        // construct new/modified TestTreeItem
        TestTreeItem *ttItem = new TestTreeItem(tcName, tcLocationAndType.m_fileName,
                                                tcLocationAndType.m_type, quickTestRootItem);
        ttItem->setLine(tcLocationAndType.m_line);
        ttItem->setColumn(tcLocationAndType.m_column);
        ttItem->setMainFile(doc->fileName());

        foreach (const QString &func, testFunctions.keys()) {
            const TestCodeLocationAndType locationAndType = testFunctions.value(func);
            TestTreeItem *ttSub = new TestTreeItem(func, locationAndType.m_fileName,
                                                   locationAndType.m_type, ttItem);
            ttSub->setLine(locationAndType.m_line);
            ttSub->setColumn(locationAndType.m_column);
            ttItem->appendChild(ttSub);
        }

        // update model and internal map
        const QString fileName(tcLocationAndType.m_fileName);
        const QmlJS::Document::Ptr qmlDoc =
                QmlJSTools::Internal::ModelManager::instance()->snapshot().document(fileName);
        QString proFile;
        QList<CppTools::ProjectPart::Ptr> ppList = cppMM->projectPart(doc->fileName());
        if (ppList.size())
            proFile = ppList.at(0)->projectFile;

        if (m_quickDocMap.contains(fileName)) {
            for (int i = 0; i < quickTestRootItem->childCount(); ++i) {
                if (quickTestRootItem->child(i)->filePath() == fileName) {
                    m_model->modifyQuickTestSubtree(i, ttItem);
                    TestInfo ti(tcName, testFunctions.keys(), 0, qmlDoc->editorRevision());
                    ti.setReferencingFile(doc->fileName());
                    ti.setProfile(proFile);
                    m_quickDocMap.insert(fileName, ti);
                    break;
                }
            }
            delete ttItem;
        } else {
            // if it was formerly unnamed remove the respective items
            if (m_quickDocMap.contains(tr(Constants::UNNAMED_QUICKTESTS))) {
                if (m_model->removeUnnamedQuickTests(d->fileName())) {
                    // make sure m_quickDocMap does not have a inconsistent state now
                    TestInfo unnamedInfo = m_quickDocMap[tr(Constants::UNNAMED_QUICKTESTS)];
                    QStringList functions = unnamedInfo.testFunctions();
                    foreach (const QString &func, testFunctions.keys())
                        if (functions.contains(func))
                            functions.removeOne(func);
                    unnamedInfo.setTestFunctions(functions);
                    if (functions.size() == 0)
                        m_quickDocMap.remove(tr(Constants::UNNAMED_QUICKTESTS));
                    else
                        m_quickDocMap.insert(tr(Constants::UNNAMED_QUICKTESTS), unnamedInfo);
                }
            }

            m_model->addQuickTest(ttItem);
            TestInfo ti(tcName, testFunctions.keys(), 0, qmlDoc->editorRevision());
            ti.setReferencingFile(doc->fileName());
            ti.setProfile(proFile);
            m_quickDocMap.insert(tcLocationAndType.m_fileName, ti);
        }
    }
}

void TestCodeParser::onCppDocumentUpdated(const CPlusPlus::Document::Ptr &doc)
{
    if (!m_currentProject)
        return;
    const QString fileName = doc->fileName();
    if (m_cppDocMap.contains(fileName)) {
        if (m_cppDocMap[fileName].revision() == doc->revision()
                && m_cppDocMap[fileName].editorRevision() == doc->editorRevision()) {
            qDebug("Skipped due revision equality"); // added to verify if this ever happens..
            return;
        }
    } else if (!m_currentProject->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        return;
    }
    checkDocumentForTestCode(doc);
}

void TestCodeParser::onQmlDocumentUpdated(const QmlJS::Document::Ptr &doc)
{
    if (!m_currentProject)
        return;
    const QString fileName = doc->fileName();
    if (m_quickDocMap.contains(fileName)) {
        if ((int)m_quickDocMap[fileName].editorRevision() == doc->editorRevision()) {
            qDebug("Skipped due revision equality (QML)"); // added to verify this ever happens....
            return;
        }
    } else if (!m_currentProject->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
        // what if the file is not listed inside the pro file, but will be used anyway?
        return;
    }
    const CPlusPlus::Snapshot snapshot = CppTools::CppModelManager::instance()->snapshot();
    if (m_quickDocMap.contains(fileName)
            && snapshot.contains(m_quickDocMap[fileName].referencingFile())) {
        checkDocumentForTestCode(snapshot.document(m_quickDocMap[fileName].referencingFile()));
    }
    if (!m_quickDocMap.contains(tr(Constants::UNNAMED_QUICKTESTS)))
        return;

    // special case of having unnamed TestCases
    TestTreeItem *unnamed = m_model->unnamedQuickTests();
    for (int row = 0, count = unnamed->childCount(); row < count; ++row) {
        const TestTreeItem *child = unnamed->child(row);
        if (fileName == child->filePath()) {
            if (snapshot.contains(child->mainFile()))
                checkDocumentForTestCode(snapshot.document(child->mainFile()));
            break;
        }
    }
}

void TestCodeParser::removeFiles(const QStringList &files)
{
    foreach (const QString &file, files)
        removeTestsIfNecessary(file);
}

void TestCodeParser::scanForTests(const QStringList &fileList)
{
    QStringList list;
    if (fileList.isEmpty()) {
        list = m_currentProject->files(ProjectExplorer::Project::AllFiles);
    } else {
        list << fileList;
    }

    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    CPlusPlus::Snapshot snapshot = cppMM->snapshot();

    foreach (const QString &file, list) {
        if (snapshot.contains(file)) {
            CPlusPlus::Document::Ptr doc = snapshot.find(file).value();
            checkDocumentForTestCode(doc);
        }
    }
}

void TestCodeParser::clearMaps()
{
    m_cppDocMap.clear();
    m_quickDocMap.clear();
}

void TestCodeParser::removeTestsIfNecessary(const QString &fileName)
{
    // check if this file was listed before and remove if necessary (switched config,...)
    if (m_cppDocMap.contains(fileName)) {
        m_cppDocMap.remove(fileName);
        m_model->removeAutoTestSubtreeByFilePath(fileName);
    } else { // handle Qt Quick Tests
        QList<QString> toBeRemoved;
        foreach (const QString &file, m_quickDocMap.keys()) {
            if (file == fileName) {
                toBeRemoved.append(file);
                continue;
            }
            const TestInfo info = m_quickDocMap.value(file);
            if (info.referencingFile() == fileName)
                toBeRemoved.append(file);
        }
        foreach (const QString &file, toBeRemoved) {
            m_quickDocMap.remove(file);
            m_model->removeQuickTestSubtreeByFilePath(file);
        }
        // unnamed Quick Tests must be handled separately
        QSet<QString> filePaths;
        QList<QString> functionNames;
        if (TestTreeItem *unnamedQT = m_model->unnamedQuickTests()) {
            for (int i = 0; i < unnamedQT->childCount(); ++i) {
                const TestTreeItem *child = unnamedQT->child(i);
                if (child->mainFile() == fileName) {
                    filePaths.insert(child->filePath());
                    functionNames.append(child->name());
                }
            }
            foreach (const QString &file, filePaths)
                m_model->removeUnnamedQuickTests(file);
            // update info map
            TestInfo unnamedInfo = m_quickDocMap[tr(Constants::UNNAMED_QUICKTESTS)];
            QStringList functions = unnamedInfo.testFunctions();
            foreach (const QString &func, functionNames)
                functions.removeOne(func);
            unnamedInfo.setTestFunctions(functions);
            if (functions.size() == 0)
                m_quickDocMap.remove(tr(Constants::UNNAMED_QUICKTESTS));
            else
                m_quickDocMap.insert(tr(Constants::UNNAMED_QUICKTESTS), unnamedInfo);
        }
    }
}


void TestCodeParser::removeTestsIfNecessaryByProFile(const QString &proFile)
{
    QList<QString> fl;
    foreach (const QString &fn, m_cppDocMap.keys()) {
        if (m_cppDocMap[fn].proFile() == proFile)
            fl.append(fn);
    }
    foreach (const QString &fn, fl) {
        m_cppDocMap.remove(fn);
        m_model->removeAutoTestSubtreeByFilePath(fn);
    }
    fl.clear();
    foreach (const QString &fn, m_quickDocMap.keys()) {
       if (m_quickDocMap[fn].proFile() == proFile)
           fl.append(fn);
    }
    foreach (const QString &fn, fl) {
        m_quickDocMap.remove(fn);
        m_model->removeQuickTestSubtreeByFilePath(fn);
    }
    // handle unnamed Quick Tests
    fl.clear(); // will now be re-used as function names storage
    QSet<QString> filePaths;
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    if (TestTreeItem *unnamedQT = m_model->unnamedQuickTests()) {
        for (int i = 0; i < unnamedQT->childCount(); ++i) {
            const TestTreeItem *child = unnamedQT->child(i);
            QList<CppTools::ProjectPart::Ptr> ppList = cppMM->projectPart(child->mainFile());
            if (ppList.size() && ppList.at(0)->projectFile == proFile) {
                filePaths.insert(child->filePath());
                fl.append(child->name());
            }
        }
    }
    foreach (const QString &fp, filePaths) {
        m_model->removeUnnamedQuickTests(fp);
    }
    // update info map
    TestInfo unnamedInfo = m_quickDocMap[tr(Constants::UNNAMED_QUICKTESTS)];
    QStringList functions = unnamedInfo.testFunctions();
    foreach (const QString &func, fl)
        functions.removeOne(func);
    unnamedInfo.setTestFunctions(functions);
    if (functions.size() == 0)
        m_quickDocMap.remove(tr(Constants::UNNAMED_QUICKTESTS));
    else
        m_quickDocMap.insert(tr(Constants::UNNAMED_QUICKTESTS), unnamedInfo);
}

void TestCodeParser::onProFileEvaluated()
{
    if (!m_currentProject)
        return;

    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    const QList<CppTools::ProjectPart::Ptr> pp = cppMM->projectInfo(m_currentProject).projectParts();
    foreach (const CppTools::ProjectPart::Ptr &p, pp) {
        if (!p->selectedForBuilding)
            removeTestsIfNecessaryByProFile(p->projectFile);
        else {
            QStringList files;
            foreach (auto projectFile, p->files)
                files.append(projectFile.path);
            scanForTests(files);
        }
    }
}

} // namespace Internal
} // namespace Autotest
