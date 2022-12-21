// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmlprofiler/qmlprofilerdetailsrewriter.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerDetailsRewriterTest : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerDetailsRewriterTest(QObject *parent = nullptr);

private slots:
    void testMissingModelManager();
    void testRequestDetailsForLocation();
    void testGetLocalFile();
    void testPopulateFileFinder();

private:
    QmlJS::ModelManagerInterface *m_modelManager = nullptr;
    QmlProfilerDetailsRewriter m_rewriter;
    bool m_rewriterDone = false;

    void seedRewriter();
};

} // namespace Internal
} // namespace QmlProfiler
