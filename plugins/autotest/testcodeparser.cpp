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
    const ProjectExplorer::SessionManager *session = ProjectExplorer::SessionManager::instance();
    if (!session || !session->hasProjects())
        return;

    m_currentProject = session->startupProject();
    if (!m_currentProject)
        return;
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

    foreach (const CPlusPlus::Document::Include inc, includes) {
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
        foreach (const QString include, allIncludes) {
            if (include.endsWith(QLatin1String("QtTest/qtest.h"))) {
                return true;
            }
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

static QString testClass(const CPlusPlus::Document::Ptr &doc)
{
    static QByteArray qtTestMacros[] = {"QTEST_MAIN", "QTEST_APPLESS_MAIN", "QTEST_GUILESS_MAIN"};
    QString tC;
    const QList<CPlusPlus::Document::MacroUse> macros = doc->macroUses();

    foreach (const CPlusPlus::Document::MacroUse macro, macros) {
        if (!macro.isFunctionLike())
            continue;
        const QByteArray name = macro.macro().name();
        if (name == qtTestMacros[0] || name == qtTestMacros[1] || name == qtTestMacros[2]) {
            CPlusPlus::Document::Block arg = macro.arguments().at(0);
            tC = QLatin1String(getFileContent(doc->fileName()).mid(arg.bytesBegin(),
                                                                   arg.bytesEnd() - arg.bytesBegin()));
            break;
        }
    }
    return tC;
}

/****** end of helpers ******/

void TestCodeParser::checkDocumentForTestCode(CPlusPlus::Document::Ptr doc)
{
    const QString file = doc->fileName();
    const CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();

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
            const QMap<QString, TestCodeLocation> privSlots = myVisitor.privateSlots();
            foreach (const QString privS, privSlots.keys()) {
                const TestCodeLocation location = privSlots.value(privS);
                TestTreeItem *ttSub = new TestTreeItem(privS, location.m_fileName,
                                                       TestTreeItem::TEST_FUNCTION, ttItem);
                ttSub->setLine(location.m_line);
                ttSub->setColumn(location.m_column);
                ttItem->appendChild(ttSub);
            }

            // TODO refactoring?
            // update model and internal map
            TestInfo info;
            int count;
            if (m_cppDocMap.contains(file)) {
                info = m_cppDocMap.value(file);
                count = autoTestRootItem->childCount();
                for (int i = 0; i < count; ++i) {
                    TestTreeItem *currentItem = autoTestRootItem->child(i);
                    if (currentItem->filePath() == file) {
                        m_model->modifyAutoTestSubtree(i, ttItem);
                        m_cppDocMap.insert(file, TestInfo(tc, privSlots.keys(),
                                                          doc->revision(),
                                                          doc->editorRevision()));
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
                            m_cppDocMap.insert(declFileName, ti);
                            break;
                        }
                    }
                }
                delete ttItem;
            } else {
                m_model->addAutoTest(ttItem);
                m_cppDocMap.insert(file, TestInfo(tc, privSlots.keys(), doc->revision(),
                                                  doc->editorRevision()));
                if (declFileName != file) {
                    TestInfo ti(tc, privSlots.keys(), doc->revision(), doc->editorRevision());
                    ti.setReferencingFile(file);
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

void TestCodeParser::onDocumentUpdated(CPlusPlus::Document::Ptr doc)
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

void TestCodeParser::removeFiles(const QStringList &files)
{
    foreach (const QString file, files) {
        if (m_cppDocMap.contains(file)) {
            m_cppDocMap.remove(file);
            m_model->removeAutoTestSubtreeByFilePath(file);
        }
    }
}

void TestCodeParser::scanForTests()
{
    const QStringList list = m_currentProject->files(ProjectExplorer::Project::AllFiles);
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    CPlusPlus::Snapshot snapshot = cppMM->snapshot();

    foreach (const QString file, list) {
        if (snapshot.contains(file)) {
            CPlusPlus::Document::Ptr doc = snapshot.find(file).value();
            checkDocumentForTestCode(doc);
        }
    }
}

void TestCodeParser::clearMaps()
{
    m_cppDocMap.clear();
}

} // namespace Internal
} // namespace Autotest
