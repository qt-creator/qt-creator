/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "cpptoolsplugin.h"
#include "cppmodelmanager.h"
#include "modelmanagertesthelper.h"

#include <QtTest>
#include <QDebug>

using namespace CppTools::Internal;

typedef CPlusPlus::CppModelManagerInterface::ProjectInfo ProjectInfo;
typedef CPlusPlus::CppModelManagerInterface::ProjectPart ProjectPart;
typedef ProjectExplorer::Project Project;

namespace {
QString testDataDir(const QString& subdir, bool cleaned = true)
{
    QString path = QLatin1String(SRCDIR "/../../../tests/cppmodelmanager/testdata");
    if (!subdir.isEmpty())
        path += QLatin1String("/") + subdir;
    if (cleaned)
        return CppPreprocessor::cleanPath(path);
    else
        return path;
}

QString testIncludeDir(bool cleaned = true)
{
    return testDataDir(QLatin1String("include"), cleaned);
}

QString testFrameworksDir(bool cleaned = true)
{
    return testDataDir(QLatin1String("frameworks"), cleaned);
}
} // anonymous namespace

void CppToolsPlugin::test_modelmanager_paths()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    Project *project = helper.createProject(QLatin1String("test_modelmanager_paths"));
    ProjectInfo pi = mm->projectInfo(project);
    QCOMPARE(pi.project().data(), project);

    ProjectPart::Ptr part(new ProjectPart);
    pi.appendProjectPart(part);
    part->language = ProjectPart::CXX;
    part->qtVersion = ProjectPart::Qt5;
    part->defines = QByteArray("#define OH_BEHAVE -1\n");
    part->includePaths = QStringList() << testIncludeDir(false);
    part->frameworkPaths = QStringList() << testFrameworksDir(false);

    mm->updateProjectInfo(pi);

    QStringList includePaths = mm->includePaths();
    QCOMPARE(includePaths.size(), 1);
    QVERIFY(includePaths.contains(testIncludeDir()));

    QStringList frameworkPaths = mm->frameworkPaths();
    QCOMPARE(frameworkPaths.size(), 1);
    QVERIFY(frameworkPaths.contains(testFrameworksDir()));
}
