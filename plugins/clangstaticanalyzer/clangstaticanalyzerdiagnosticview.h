/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
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

#ifndef CLANGSTATICANALYZERDIAGNOSTICVIEW_H
#define CLANGSTATICANALYZERDIAGNOSTICVIEW_H

#include <analyzerbase/detailederrorview.h>

namespace ClangStaticAnalyzer {
namespace Internal {

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
