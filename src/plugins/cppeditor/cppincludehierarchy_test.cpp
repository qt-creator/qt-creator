/****************************************************************************
**
** Copyright (C) 2013 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
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

#include "cppeditorplugin.h"
#include "cppeditortestcase.h"
#include "cppincludehierarchymodel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cppeditor/cppeditor.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <utils/fileutils.h>

#include <QByteArray>
#include <QList>
#include <QtTest>

using namespace CPlusPlus;
using namespace CppEditor::Internal;
using namespace CppTools;

namespace {

class IncludeHierarchyTestCase: public CppEditor::Internal::Tests::TestCase
{
public:
    IncludeHierarchyTestCase(const QList<QByteArray> &sourceList,
                             int includesCount,
                             int includedByCount)
    {
        QVERIFY(succeededSoFar());

        QStringList filePaths;
        const int sourceListSize = sourceList.size();
        for (int i = 0; i < sourceListSize; ++i) {
            const QByteArray &source = sourceList.at(i);

            // Write source to file
            const QString fileName = QString::fromLatin1("%1/file%2.h").arg(QDir::tempPath())
                    .arg(i+1);
            QVERIFY(writeFile(fileName, source));

            filePaths << fileName;
        }

        // Update Code Model
        QVERIFY(parseFiles(filePaths));

        // Open Editor
        const QString fileName = QDir::tempPath() + QLatin1String("/file1.h");
        CPPEditor *editor;
        QVERIFY(openCppEditor(fileName, &editor));
        closeEditorAtEndOfTestCase(editor);

        // Test model
        CppIncludeHierarchyModel model(0);
        model.buildHierarchy(editor, fileName);
        QCOMPARE(model.rowCount(model.index(0, 0)), includesCount);
        QCOMPARE(model.rowCount(model.index(1, 0)), includedByCount);
    }
};

} // anonymous namespace

void CppEditorPlugin::test_includeHierarchyModel_simpleIncludes()
{
    QList<QByteArray> sourceList;
    sourceList.append(QByteArray("#include \"file2.h\"\n"));
    sourceList.append(QByteArray());

    IncludeHierarchyTestCase(sourceList, 1, 0);
}

void CppEditorPlugin::test_includeHierarchyModel_simpleIncludedBy()
{
    QList<QByteArray> sourceList;
    sourceList.append(QByteArray());
    sourceList.append(QByteArray("#include \"file1.h\"\n"));

    IncludeHierarchyTestCase(sourceList, 0, 1);
}

void CppEditorPlugin::test_includeHierarchyModel_simpleIncludesAndIncludedBy()
{
    QList<QByteArray> sourceList;
    sourceList.append(QByteArray("#include \"file2.h\"\n"));
    sourceList.append(QByteArray());
    sourceList.append(QByteArray("#include \"file1.h\"\n"));

    IncludeHierarchyTestCase(sourceList, 1, 1);
}
