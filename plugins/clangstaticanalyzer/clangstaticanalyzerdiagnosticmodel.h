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

#ifndef CLANGSTATICANALYZERDIAGNOSTICMODEL_H
#define CLANGSTATICANALYZERDIAGNOSTICMODEL_H

#include "clangstaticanalyzerlogfilereader.h"

#include <QAbstractListModel>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerDiagnosticModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ClangStaticAnalyzerDiagnosticModel(QObject *parent = 0);

    void addDiagnostics(const QList<Diagnostic> &diagnostics);
    void clear();

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    QList<Diagnostic> m_diagnostics;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERDIAGNOSTICMODEL_H
