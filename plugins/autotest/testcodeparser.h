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

#ifndef TESTCODEPARSER_H
#define TESTCODEPARSER_H

#include <cplusplus/CppDocument.h>

#include <QObject>
#include <QMap>

namespace ProjectExplorer {
class Project;
}

namespace Autotest {
namespace Internal {

class TestInfo;
class TestTreeModel;

class TestCodeParser : public QObject
{
    Q_OBJECT
public:
    explicit TestCodeParser(TestTreeModel *parent = 0);

signals:

public slots:
    void updateTestTree();
    void checkDocumentForTestCode(CPlusPlus::Document::Ptr doc);

    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void removeFiles(const QStringList &files);

private:
    void scanForTests();

    TestTreeModel *m_model;
    QMap<QString, TestInfo*> m_cppDocMap;
    ProjectExplorer::Project *m_currentProject;
};

} // namespace Internal
} // Autotest

#endif // TESTCODEPARSER_H
