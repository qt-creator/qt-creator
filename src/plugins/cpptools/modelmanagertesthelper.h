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

#ifndef CPPTOOLS_INTERNAL_MODELMANAGERTESTHELPER_H
#define CPPTOOLS_INTERNAL_MODELMANAGERTESTHELPER_H

#include "cppmodelmanager.h"

#include <QObject>

namespace CppTools {
namespace Internal {

class TestProject: public ProjectExplorer::Project
{
    Q_OBJECT

public:
    TestProject(const QString &name, QObject *parent);
    virtual ~TestProject();

    virtual QString displayName() const
    { return m_name; }

    virtual Core::Id id() const
    { return Core::Id::fromString(m_name); }

    virtual Core::IDocument *document() const
    { return 0; }

    virtual ProjectExplorer::IProjectManager *projectManager() const
    { return 0; }

    virtual ProjectExplorer::ProjectNode *rootProjectNode() const
    { return 0; }

    virtual QStringList files(FilesMode fileMode) const
    { Q_UNUSED(fileMode); return QStringList(); }

private:
    QString m_name;
};

class ModelManagerTestHelper: public QObject
{
    Q_OBJECT

public:
    typedef CPlusPlus::CppModelManagerInterface::ProjectInfo ProjectInfo;
    typedef ProjectExplorer::Project Project;

    explicit ModelManagerTestHelper(QObject *parent = 0);
    ~ModelManagerTestHelper();

    void cleanup();
    void verifyClean();

    Project *createProject(const QString &name);

signals:
    void aboutToRemoveProject(ProjectExplorer::Project *project);
    void projectAdded(ProjectExplorer::Project*);
};

} // namespace Internal
} // namespace CppTools

#endif // CPPTOOLS_INTERNAL_MODELMANAGERTESTHELPER_H
