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

namespace Autotest {
namespace Internal {

TestCodeParser::TestCodeParser(TestTreeModel *parent)
    : QObject(parent),
      m_model(parent),
      m_currentProject(0)
{
}

void TestCodeParser::updateTestTree()
{
    qDebug("updating TestTreeModel");
    m_model->beginResetModel();
    qDeleteAll(m_cppDocMap);
    m_cppDocMap.clear();
    QModelIndex autoTestRootIndex = m_model->index(0, 0);
    TestTreeItem *autoTestRootItem = static_cast<TestTreeItem *>(autoTestRootIndex.internalPointer());
    autoTestRootItem->removeChildren();
    m_model->endResetModel();
    ProjectExplorer::SessionManager *session = static_cast<ProjectExplorer::SessionManager *>(
                ProjectExplorer::SessionManager::instance());
    if (!session || !session->hasProjects())
        return;

    m_currentProject = session->startupProject();
    if (!m_currentProject)
        return;
    scanForTests();
}

/****** scan for QTest related stuff helpers ******/

static bool includesQtTest(const CPlusPlus::Document::Ptr &doc,
                           const CppTools::CppModelManager *cppMM)
{
    QList<CPlusPlus::Document::Include> includes = doc->resolvedIncludes();

    foreach (CPlusPlus::Document::Include inc, includes) {
        // TODO this short cut works only for #include <QtTest>
        // bad, as there could be much more different approaches
        if (inc.unresolvedFileName() == QLatin1String("QtTest")
                && inc.resolvedFileName().endsWith(QLatin1String("QtTest/QtTest"))) {
            return true;
        }
    }

    if (cppMM) {
        CPlusPlus::Snapshot snapshot = cppMM->snapshot();
        QSet<QString> includes = snapshot.allIncludesForDocument(doc->fileName());
        foreach (QString include, includes) {
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
    QList<CppTools::ProjectPart::Ptr> parts = cppMM->projectPart(fileName);
    if (parts.size() > 0) {
        QByteArray projDefines = parts.at(0)->projectDefines;
        return projDefines.contains("#define QT_TESTLIB_LIB 1");
    }
    return false;
}

static QString testClass(const CPlusPlus::Document::Ptr &doc)
{
    static QByteArray qtTestMacros[] = {"QTEST_MAIN", "QTEST_APPLESS_MAIN", "QTEST_GUILESS_MAIN"};
    QString tC;
    QList<CPlusPlus::Document::MacroUse> macros = doc->macroUses();

    foreach (CPlusPlus::Document::MacroUse macro, macros) {
        if (!macro.isFunctionLike())
            continue;
        QByteArray name = macro.macro().name();
        if (name == qtTestMacros[0] || name == qtTestMacros[1] || name == qtTestMacros[2]) {
            CPlusPlus::Document::Block arg = macro.arguments().at(0);
            CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();

            // TODO high costs....?!
            CppTools::WorkingCopy wc = cppMM->workingCopy();
            if (wc.contains(doc->fileName())) {
                QByteArray src = wc.source(doc->fileName());
                tC = QLatin1String(src.mid(arg.bytesBegin(), arg.bytesEnd() - arg.bytesBegin()));
            } else {
                QFile current(doc->fileName());
                if (current.exists() && current.open(QFile::ReadOnly)) {
                    CPlusPlus::Document::Block arg = macro.arguments().at(0);
                    if (current.seek(arg.bytesBegin())) {
                        QByteArray res = current.read(arg.bytesEnd() - arg.bytesBegin());
                        if (!res.isEmpty())
                            tC = QLatin1String(res);
                    }
                    current.close();
                }
            }
            break;
        }
    }
    return tC;
}

/****** end of helpers ******/

void TestCodeParser::checkDocumentForTestCode(CPlusPlus::Document::Ptr doc)
{
    const QString file = doc->fileName();
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();

    if (includesQtTest(doc, cppMM) && qtTestLibDefined(cppMM, file)) {
        QString tc = testClass(doc);
        if (tc.isEmpty()) {
            // one might have used an approach without macros or defined own macros
            QString src = QLatin1String(doc->utf8Source()); // does not work anymore - src is always ""
            if (src.contains(QLatin1String("QTest::qExec"))) {
                qDebug() << "Src\n===============\n" << src << "\n=============\n";
                // TODO extract the class name by using the AST - using regex is too problematic as this
                // does not take comments and typedefs or whatever into account....
                qDebug() << "Currently not supported approach... (self-defined macro / QTest::qExec() call/...)";
            }
        } else {
            QModelIndex autoTestRootIndex = m_model->index(0, 0);
            TestTreeItem *autoTestRootItem = static_cast<TestTreeItem *>(autoTestRootIndex.internalPointer());
            TestTreeItem *ttItem = new TestTreeItem(tc, file, TestTreeItem::TEST_CLASS,
                                                    autoTestRootItem);
            CPlusPlus::TypeOfExpression toe;
            toe.init(doc, cppMM->snapshot());
            CPlusPlus::Document::Ptr declaringDoc = doc;
            QList<CPlusPlus::LookupItem>  toeItems = toe(tc.toUtf8(), doc->globalNamespace());
            if (toeItems.size()) {
                CPlusPlus::Class *toeClass = toeItems.first().declaration()->asClass();
                QString declFileName = QLatin1String(toeClass->fileId()->chars(),
                                                 toeClass->fileId()->size());
                declaringDoc = cppMM->snapshot().document(declFileName);
                ttItem->setFilePath(declFileName);
                ttItem->setLine(toeClass->line());
                ttItem->setColumn(toeClass->column() - 1);
            }
            if (declaringDoc.isNull())
                return;
            TestVisitor myVisitor(tc);
            myVisitor.accept(declaringDoc->globalNamespace());
            QMap<QString, TestCodeLocation> privSlots = myVisitor.privateSlots();
            foreach (const QString privS, privSlots.keys()) {
                TestCodeLocation location = privSlots.value(privS);
                TestTreeItem *ttSub = new TestTreeItem(privS, location.m_fileName,
                                                       TestTreeItem::TEST_FUNCTION, ttItem);
                ttSub->setLine(location.m_line);
                ttSub->setColumn(location.m_column);
                ttItem->appendChild(ttSub);
            }
            if (m_cppDocMap.contains(file)) {
            TestInfo *info = m_cppDocMap[file];
                int count = autoTestRootItem->childCount();
                for (int i = 0; i < count; ++i) {
                    TestTreeItem *currentItem = autoTestRootItem->child(i);
                    if (currentItem->filePath() == file) {
                        m_model->modifyAutoTestSubtree(i, ttItem);
                        m_cppDocMap.insert(file, new TestInfo(tc, privSlots.keys(),
                                                              doc->revision(),
                                                              doc->editorRevision()));
                        break;
                    }
                }
                delete info;
                delete ttItem;
            } else {
                m_model->beginInsertRows(autoTestRootIndex, autoTestRootItem->childCount(), autoTestRootItem->childCount());
                autoTestRootItem->appendChild(ttItem);
                m_model->endInsertRows();
                m_cppDocMap.insert(file, new TestInfo(tc, privSlots.keys(),
                                                      doc->revision(), doc->editorRevision()));
            }
        }
    }
}

void TestCodeParser::onDocumentUpdated(CPlusPlus::Document::Ptr doc)
{
    if (!m_currentProject)
        return;
    QString fileName = doc->fileName();
    if (!m_cppDocMap.contains(fileName)) {
        if (!m_currentProject->files(ProjectExplorer::Project::AllFiles).contains(fileName)) {
            return;
        }
    } else {
        if (m_cppDocMap[fileName]->revision() == doc->revision()
                && m_cppDocMap[fileName]->editorRevision() == doc->editorRevision()) {
            qDebug("Skipped due revision equality"); // added to verify if this ever happens..
            return;
        }
    }
    checkDocumentForTestCode(doc);
}

void TestCodeParser::removeFiles(const QStringList &files)
{
    foreach (QString file, files) {
        if (m_cppDocMap.contains(file)) {
            TestInfo *info = m_cppDocMap.value(file);
            m_cppDocMap.remove(file);
            m_model->removeAutoTestSubtreeByFilePath(file);
            delete info;
        }
    }
}

void TestCodeParser::scanForTests()
{
    QStringList list = m_currentProject->files(ProjectExplorer::Project::AllFiles);
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    CPlusPlus::Snapshot snapshot = cppMM->snapshot();

    foreach (QString file, list) {
        if (snapshot.contains(file)) {
            CPlusPlus::Document::Ptr doc = snapshot.find(file).value();
            checkDocumentForTestCode(doc);
        }
    }
}

} // namespace Internal
} // namespace Autotest
