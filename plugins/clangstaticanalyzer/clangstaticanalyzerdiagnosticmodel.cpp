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

#include "clangstaticanalyzerdiagnosticmodel.h"

#include "clangstaticanalyzerutils.h"

#include <QCoreApplication>

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerDiagnosticModel::ClangStaticAnalyzerDiagnosticModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ClangStaticAnalyzerDiagnosticModel::addDiagnostics(const QList<Diagnostic> &diagnostics)
{
    beginInsertRows(QModelIndex(), m_diagnostics.size(),
                    m_diagnostics.size() + diagnostics.size() - 1 );
    m_diagnostics += diagnostics;
    endInsertRows();
}

void ClangStaticAnalyzerDiagnosticModel::clear()
{
    beginResetModel();
    m_diagnostics.clear();
    endResetModel();
}

int ClangStaticAnalyzerDiagnosticModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_diagnostics.count();
}

static QString createDiagnosticToolTipString(const Diagnostic &diagnostic)
{
    typedef QPair<QString, QString> StringPair;
    QList<StringPair> lines;

    if (!diagnostic.category.isEmpty()) {
        lines << qMakePair(
                     QCoreApplication::translate("ClangStaticAnalyzer::Diagnostic", "Category:"),
                     diagnostic.category);
    }

    if (!diagnostic.type.isEmpty()) {
        lines << qMakePair(
                     QCoreApplication::translate("ClangStaticAnalyzer::Diagnostic", "Type:"),
                     diagnostic.type);
    }

    if (!diagnostic.issueContext.isEmpty() && !diagnostic.issueContextKind.isEmpty()) {
        lines << qMakePair(
                     QCoreApplication::translate("ClangStaticAnalyzer::Diagnostic", "Context:"),
                     diagnostic.issueContextKind + QLatin1Char(' ') + diagnostic.issueContext);
    }

    lines << qMakePair(
        QCoreApplication::translate("ClangStaticAnalyzer::Diagnostic", "Location:"),
                createFullLocationString(diagnostic.location));

    QString html = QLatin1String("<html>"
                   "<head>"
                   "<style>dt { font-weight:bold; } dd { font-family: monospace; }</style>\n"
                   "<body><dl>");

    foreach (const StringPair &pair, lines) {
        html += QLatin1String("<dt>");
        html += pair.first;
        html += QLatin1String("</dt><dd>");
        html += pair.second;
        html += QLatin1String("</dd>\n");
    }
    html += QLatin1String("</dl></body></html>");
    return html;
}

QVariant ClangStaticAnalyzerDiagnosticModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.parent().isValid())
        return QVariant();

    const int row = index.row();
    if (row < 0 || row >= m_diagnostics.size())
        return QVariant();

    const Diagnostic diagnostic = m_diagnostics.at(row);

    if (role == Qt::DisplayRole)
        return QString(QLatin1String("Some specific diagnostic")); // TODO: Remove?
    else if (role == Qt::ToolTipRole)
        return createDiagnosticToolTipString(diagnostic);
    else if (role == Qt::UserRole)
        return QVariant::fromValue<Diagnostic>(diagnostic);

    return QVariant();
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
