/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef TESTCODE_H
#define TESTCODE_H

#include <qmljs/qmljsdocument.h>
#include <cplusplus/CppDocument.h>

#include <QDateTime>
#include <QPointer>
#include <QTimer>

class TestCollectionPrivate;

namespace Core {
    class IEditor;
}

namespace TextEditor {
class ITextEditor;
class BaseTextEditor;
}

namespace QmlJS {
class ModelManagerInterface;
}

namespace CPlusPlus {
    class CppModelManagerInterface;
}

class SystemTestCodeSync;

class TestFunctionInfo : public QObject
{
    Q_OBJECT
public:
    TestFunctionInfo();
    TestFunctionInfo(TestFunctionInfo &other);
    virtual ~TestFunctionInfo();
    void reset();

    static bool validFunctionName(const QString &funcName);
    bool needsRevision();

    TestFunctionInfo& operator=(const TestFunctionInfo &other);

    void setFunctionName(const QString &nm) { m_functionName = nm; }
    QString functionName() const { return m_functionName; }

    void setManualTest(bool isManual) { m_isManualTest = isManual; }
    bool isManualTest() const { return m_isManualTest; }

    void setTestStart(int s) { m_testStart = s; }
    int testStart() const { return m_testStart; }

    void setTestStartLine(int s) { m_testStartLine = s; }
    int testStartLine() const { return m_testStartLine; }

    void setTestEnd(int e) { m_testEnd = e; }
    int testEnd() const { return m_testEnd; }

    void setDeclarationStart(int s) { m_declStart = s; }
    int declarationStart() const { return m_declStart; }

    void setDeclarationStartLine(int s) { m_declStartLine = s; }
    int declarationStartLine() const { return m_declStartLine; }

    void setDeclarationEnd(int e) { m_declEnd = e; }
    int declarationEnd() const { return m_declEnd; }

    void setTestGroups(const QString &grps) { m_testGroups = grps; }
    QString testGroups() const { return m_testGroups; }

    void processDocBlock(const QStringList &docBlock);

private:
    QString m_functionName;
    bool m_isManualTest;
    int m_testStart;
    int m_testStartLine;
    int m_testEnd;
    int m_declStart;
    int m_declStartLine;
    int m_declEnd;
    QString m_testGroups;
};

class TestCode : public QObject
{
    Q_OBJECT
public:
    enum TestType {
        TypeUnknownTest     = 0x1,
        TypeUnitTest        = 0x2,
        TypeIntegrationTest = 0x4,
        TypePerformanceTest = 0x8,
        TypeSystemTest      = 0x10
    };
    Q_DECLARE_FLAGS(TestTypes, TestType)

    TestCode(const QString &basePath, const QString &extraPath, const QString &fileName);
    virtual ~TestCode();

    uint testFunctionCount();
    void clearTestFunctions();
    TestFunctionInfo *currentTestFunction();
    TestFunctionInfo *testFunction(int index);
    TestFunctionInfo *findFunction(int line, bool next);
    TestFunctionInfo *findFunction(const QString &funcName);
    bool testFunctionExists(QString funcName);
    bool requirementIsTested(const QString &reqID);

    QString visualBasePath();
    QString actualBasePath();
    QString externalBasePath();
    QString fullVisualSuitePath(bool componentViewMode) const;

    QString baseFileName() const; // just the file name part
    QString targetFileName(const QString &buildPath) const;
    QString actualFileName() const;
    QString visualFileName(bool componentViewMode) const;

    QString projectFileName();
    QString execFileName();

    QString testedComponent() const;
    QString testedClass() const;
    QString testedFile() const;
    QString testCase() const;
    TestType testType() const;
    QString testTypeString() const;

    void save(bool force = false);
    bool openTestInEditor(const QString &testFunction);
    Core::IEditor *editor();

    bool gotoLine(int lineNumber);
    void addTestFunction(const QString &newFuncName, const QString &newFuncHeader,
        bool insertAtCursorPosition);
    bool hasUnsavedChanges() const;
    bool isInitialized() const;
    bool isErrored() const;
    void setDocument(QmlJS::Document::Ptr doc);
    void setDocument(CPlusPlus::Document::Ptr doc);

signals:
    void testChanged(TestCode *testCode);
    void testRemoved(TestCode *testCode);
    void changed();

public slots:
    void onContextHelpIdRequested(TextEditor::ITextEditor *editor, int position);
    void parseDocument();

private:
    friend class SystemTestCodeSync;
    friend class UnitTestCodeSync;

    bool parseComments(const QString&);
    TestFunctionInfo *processFunction(const QString&, int, int, int);
    void setManualTest(int);
    static bool validFunctionName(const QString &funcName);

    QString m_testedComponent;
    QString m_testedFile;
    QString m_testedClass;
    QString m_testCase;
    QString m_basePath;
    QString m_externalPath;
    QString m_fileName;

    TestType m_testType;
    QList<TestFunctionInfo*> m_testFunctions;
    bool m_hasChanged;
    QPointer<TextEditor::BaseTextEditor> m_codeEditor;
    QTimer m_editTimer;
    QDateTime m_lastModified;
    QFileInfo *m_fileInfo;
    QmlJS::Document::Ptr m_qmlJSDoc;
    CPlusPlus::Document::Ptr m_cppDoc;
    QTimer m_parseTimer;
    bool m_initialized;
    bool m_errored;
};

class TestCollection : public QObject
{
    Q_OBJECT
public:
    TestCollection();
    virtual ~TestCollection();

    TestCode *currentEditedTest();
    void setCurrentEditedTest(TestCode *code);

    TestCode *findCode(const QString &fileName, const QString &basePath, const QString &extraPath);
    TestCode *findCodeByVisibleName(const QString &fileName, bool componentMode);
    TestCode *findCodeByTestCaseName(const QString &testCaseName);

    TestCode *testCode(int index);
    int count();
    void removeAll();

    void addPath(const QString &srcPath);
    void addExtraPath(const QString &srcPath);
    void removePath(const QString &srcPath);

    QStringList testFiles();
    QStringList manualTests(const QString &startPath, bool componentMode);

signals:
    void testChanged(TestCode *testCode);
    void testRemoved(TestCode *testCode);
    void changed();

private:
    static TestCollectionPrivate *d;
    static int m_refCount;
};


class TestCollectionPrivate : public QObject
{
    Q_OBJECT
public:
    TestCollectionPrivate();
    virtual ~TestCollectionPrivate();

    TestCode *currentEditedTest();
    void setCurrentEditedTest(TestCode *code);

    TestCode *findCode(const QString &fileName, const QString &basePath, const QString &extraPath);
    TestCode *findCodeByVisibleName(const QString &fileName, bool componentMode);
    TestCode *findCodeByTestCaseName(const QString &testCaseName);

    void removeCode(const QString &fileName);
    TestCode *testCode(int index);
    int count();

    QStringList testFiles();
    QStringList manualTests(const QString &startPath, bool componentMode);

public slots:
    void addPath(const QString &srcPath);
    void addExtraPath(const QString &srcPath);
    void removePath(const QString &srcPath);
    void removeAll();
    void initModelManager();
    void onDocumentUpdated(QmlJS::Document::Ptr doc);
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);

private:
    void scanTests(const QString &suitePath);
    void scanPotentialFiles(const QString &suitePath);
    bool isUnitTestCase(const CPlusPlus::Document::Ptr &doc, const CPlusPlus::Snapshot &snapshot);
    bool isUnitTestCase(const CPlusPlus::Document::Ptr &doc,
        const CPlusPlus::Snapshot &snapshot, QStringList &visited);

signals:
    void testChanged(TestCode *testCode);
    void testRemoved(TestCode *testCode);
    void changed();

private:
    QString m_currentScanRoot;
    QString m_currentExtraBase;
    QList<TestCode*> m_codeList;
    QPointer<TestCode> m_currentEditedTest;

    QmlJS::ModelManagerInterface *m_qmlJSModelManager;
    CPlusPlus::CppModelManagerInterface *m_cppModelManager;

    QMap<QString, TestCode*> m_qttDocumentMap;
    QMap<QString, TestCode*> m_cppDocumentMap;

    QList<QString> m_unitTestScanList;
    QList<QString> m_systemTestScanList;
};

#endif  // TESTCODE_H
