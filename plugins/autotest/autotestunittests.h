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

#ifndef AUTOTESTUNITTESTS_H
#define AUTOTESTUNITTESTS_H

#include <QObject>
#include <QTemporaryDir>

namespace CppTools { namespace Tests { class TemporaryCopiedDir; } }

namespace Autotest {
namespace Internal {

class TestTreeModel;

class AutoTestUnitTests : public QObject
{
    Q_OBJECT
public:
    explicit AutoTestUnitTests(TestTreeModel *model, QObject *parent = 0);

signals:

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCodeParser();
    void testCodeParser_data();

private:
    TestTreeModel *m_model;
    CppTools::Tests::TemporaryCopiedDir *m_tmpDir;
    bool m_isQt4;
};

} // namespace Internal
} // namespace Autotest

#endif // AUTOTESTUNITTESTS_H
