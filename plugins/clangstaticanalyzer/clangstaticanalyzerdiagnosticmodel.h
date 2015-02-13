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
    QList<Diagnostic> diagnostics() const { return m_diagnostics; }
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
