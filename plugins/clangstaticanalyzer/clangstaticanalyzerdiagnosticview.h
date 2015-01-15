/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef CLANGSTATICANALYZERDIAGNOSTICVIEW_H
#define CLANGSTATICANALYZERDIAGNOSTICVIEW_H

#include <analyzerbase/detailederrorview.h>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerDiagnosticView : public Analyzer::DetailedErrorView
{
    Q_OBJECT

public:
    ClangStaticAnalyzerDiagnosticView(QWidget *parent = 0);

private:
    void contextMenuEvent(QContextMenuEvent *e);

    QAction *m_copyAction;
};

class ClangStaticAnalyzerDiagnosticDelegate : public Analyzer::DetailedErrorDelegate
{
public:
    ClangStaticAnalyzerDiagnosticDelegate(QListView *parent);

    SummaryLineInfo summaryInfo(const QModelIndex &index) const;
    void copy();

private:
    QWidget *createDetailsWidget(const QFont &font, const QModelIndex &index,
                                 QWidget *parent) const;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERDIAGNOSTICVIEW_H
