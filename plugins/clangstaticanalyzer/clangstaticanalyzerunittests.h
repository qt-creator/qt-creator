/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef CLANGSTATICANALYZERUNITTESTS_H
#define CLANGSTATICANALYZERUNITTESTS_H

#include <QObject>
#include <QTemporaryDir>

namespace CppTools { namespace Tests { class TemporaryCopiedDir; } }

namespace ClangStaticAnalyzer {
namespace Internal {
class ClangStaticAnalyzerTool;

class ClangStaticAnalyzerUnitTests : public QObject
{
    Q_OBJECT

public:
    ClangStaticAnalyzerUnitTests(ClangStaticAnalyzerTool *analyzerTool, QObject *parent = 0);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testProject();
    void testProject_data();

private:
    ClangStaticAnalyzerTool * const m_analyzerTool;
    CppTools::Tests::TemporaryCopiedDir *m_tmpDir;
};

} // namespace Internal
} // namespace ClangStaticAnalyzerPlugin

#endif // Include guard

