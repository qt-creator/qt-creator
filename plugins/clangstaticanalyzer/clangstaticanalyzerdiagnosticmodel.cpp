/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
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

#include "clangstaticanalyzerdiagnosticmodel.h"

#include "clangstaticanalyzerprojectsettingsmanager.h"
#include "clangstaticanalyzerutils.h"

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFileInfo>

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
                     diagnostic.category.toHtmlEscaped());
    }

    if (!diagnostic.type.isEmpty()) {
        lines << qMakePair(
                     QCoreApplication::translate("ClangStaticAnalyzer::Diagnostic", "Type:"),
                     diagnostic.type.toHtmlEscaped());
    }

    if (!diagnostic.issueContext.isEmpty() && !diagnostic.issueContextKind.isEmpty()) {
        lines << qMakePair(
                     QCoreApplication::translate("ClangStaticAnalyzer::Diagnostic", "Context:"),
                     diagnostic.issueContextKind.toHtmlEscaped() + QLatin1Char(' ')
                     + diagnostic.issueContext.toHtmlEscaped());
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


ClangStaticAnalyzerDiagnosticFilterModel::ClangStaticAnalyzerDiagnosticFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    // So that when a user closes and re-opens a project and *then* clicks "Suppress",
    // we enter that information into the project settings.
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::projectAdded, this,
            [this](ProjectExplorer::Project *project) {
                if (!m_project && project->projectDirectory() == m_lastProjectDirectory)
                    setProject(project);
            });
}

void ClangStaticAnalyzerDiagnosticFilterModel::setProject(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    if (m_project) {
        disconnect(ProjectSettingsManager::getSettings(m_project),
                   &ProjectSettings::suppressedDiagnosticsChanged, this,
                   &ClangStaticAnalyzerDiagnosticFilterModel::handleSuppressedDiagnosticsChanged);
    }
    m_project = project;
    m_lastProjectDirectory = m_project->projectDirectory();
    connect(ProjectSettingsManager::getSettings(m_project),
            &ProjectSettings::suppressedDiagnosticsChanged,
            this, &ClangStaticAnalyzerDiagnosticFilterModel::handleSuppressedDiagnosticsChanged);
    handleSuppressedDiagnosticsChanged();
}

void ClangStaticAnalyzerDiagnosticFilterModel::addSuppressedDiagnostic(
        const SuppressedDiagnostic &diag)
{
    QTC_ASSERT(!m_project, return);
    m_suppressedDiagnostics << diag;
    invalidate();
}

bool ClangStaticAnalyzerDiagnosticFilterModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent);
    const Diagnostic diag = static_cast<ClangStaticAnalyzerDiagnosticModel *>(sourceModel())
            ->diagnostics().at(sourceRow);
    foreach (const SuppressedDiagnostic &d, m_suppressedDiagnostics) {
        if (d.description != diag.description)
            continue;
        QString filePath = d.filePath.toString();
        QFileInfo fi(filePath);
        if (fi.isRelative())
            filePath = m_lastProjectDirectory.toString() + QLatin1Char('/') + filePath;
        if (filePath == diag.location.filePath)
            return false;
    }
    return true;
}

void ClangStaticAnalyzerDiagnosticFilterModel::handleSuppressedDiagnosticsChanged()
{
    QTC_ASSERT(m_project, return);
    m_suppressedDiagnostics
            = ProjectSettingsManager::getSettings(m_project)->suppressedDiagnostics();
    invalidate();
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
