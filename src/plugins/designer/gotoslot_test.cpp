/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "formeditorplugin.h"

#if QT_VERSION < 0x050000
#include <QtTest>
#else
#include "formeditorw.h"

#include <coreplugin/testdatadir.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanager.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <QDesignerFormEditorInterface>
#include <QDesignerIntegrationInterface>
#include <QStringList>
#include <QtTest>

using namespace Core;
using namespace Core::Internal::Tests;
using namespace CppTools;
using namespace CPlusPlus;
using namespace Designer;
using namespace Designer::Internal;

namespace {

class MyTestDataDir : public Core::Internal::Tests::TestDataDir {
public:
    MyTestDataDir(const QString &dir)
        : TestDataDir(QLatin1String(SRCDIR "/../../../tests/designer/") + dir)
    {}
};

bool containsSymbol(Scope *scope, const QString &functionName)
{
    Overview oo;
    for (int i = 0, end = scope->memberCount(); i < end; ++i) {
        Symbol *symbol = scope->memberAt(i);
        const QString symbolName = oo.prettyName(symbol->name());
        if (symbolName == functionName)
            return true;
    }
    return false;
}

class GoToSlotTest
{
public:
    GoToSlotTest() : m_modelManager(CppModelManagerInterface::instance()) { cleanup(); }
    ~GoToSlotTest() { cleanup(); }

    void run() const
    {
        MyTestDataDir testData(QLatin1String("gotoslot_withoutProject"));
        const QString cppFile = testData.file(QLatin1String("form.cpp"));
        const QString hFile = testData.file(QLatin1String("form.h"));
        const QString uiFile = testData.file(QLatin1String("form.ui"));
        const QStringList files = QStringList() << cppFile << hFile << uiFile;

        const QString functionName = QLatin1String("on_pushButton_clicked");
        const QString qualifiedFunctionName = QLatin1String("Form::") + functionName;

        foreach (const QString &file, files)
            QVERIFY(EditorManager::openEditor(file));
        QCOMPARE(EditorManager::documentModel()->openedDocuments().size(), files.size());
        while (!m_modelManager->snapshot().contains(cppFile)
                 || !m_modelManager->snapshot().contains(hFile)) {
            QApplication::processEvents();
        }

        // Checks before
        Document::Ptr cppDocumentBefore = m_modelManager->snapshot().document(cppFile);
        QCOMPARE(cppDocumentBefore->globalSymbolCount(), 2U);
        QVERIFY(!containsSymbol(cppDocumentBefore->globalNamespace(), qualifiedFunctionName));

        Document::Ptr hDocumentBefore = m_modelManager->snapshot().document(hFile);
        QCOMPARE(hDocumentBefore->globalSymbolAt(1)->asScope()->memberCount(), 3U);
        QVERIFY(!containsSymbol(hDocumentBefore->globalSymbolAt(1)->asScope(), functionName));

        // Execute "Go To Slot"
        FormEditorW *few = FormEditorW::instance();
        QDesignerIntegrationInterface *integration = few->designerEditor()->integration();
        QVERIFY(integration);
        integration->emitNavigateToSlot(QLatin1String("pushButton"), QLatin1String("clicked()"),
                                        QStringList());
        QApplication::processEvents();

        // Checks after
        m_modelManager->updateSourceFiles(QStringList() << cppFile << hFile).waitForFinished();

        QCOMPARE(EditorManager::currentDocument()->filePath(), cppFile);
        QVERIFY(EditorManager::currentDocument()->isModified());

        Document::Ptr cppDocumentAfter = m_modelManager->snapshot().document(cppFile);
        QCOMPARE(cppDocumentAfter->globalSymbolCount(), 3U);
        QVERIFY(containsSymbol(cppDocumentAfter->globalNamespace(), qualifiedFunctionName));

        Document::Ptr hDocumentAfter = m_modelManager->snapshot().document(hFile);
        QCOMPARE(hDocumentAfter->globalSymbolAt(1)->asScope()->memberCount(), 4U);
        QVERIFY(containsSymbol(hDocumentAfter->globalSymbolAt(1)->asScope(), functionName));
    }

private:
    void cleanup()
    {
        EditorManager::instance()->closeAllEditors(/*askAboutModifiedEditors =*/ false);
        QVERIFY(EditorManager::documentModel()->openedDocuments().isEmpty());

        m_modelManager->GC();
        QVERIFY(m_modelManager->snapshot().isEmpty());
    }

private:
    CppModelManagerInterface *m_modelManager;
};

} // anonymous namespace
#endif

/// Check: Executes "Go To Slot..." on a QPushButton in a *.ui file and checks if the respective
/// header and source files are updated.
void Designer::Internal::FormEditorPlugin::test_gotoslot_withoutProject()
{
#if QT_VERSION >= 0x050000
    GoToSlotTest test;
    test.run();
#else
    QSKIP("Available only with >= Qt5", SkipSingle);
#endif
}
