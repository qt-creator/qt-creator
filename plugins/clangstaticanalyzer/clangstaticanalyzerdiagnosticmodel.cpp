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

#include "clangstaticanalyzerdiagnosticview.h"
#include "clangstaticanalyzerprojectsettingsmanager.h"
#include "clangstaticanalyzerutils.h"

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFileInfo>

#include <cmath>

namespace ClangStaticAnalyzer {
namespace Internal {

class DiagnosticItem : public Utils::TreeItem
{
public:
    DiagnosticItem(const Diagnostic &diag);

    Diagnostic diagnostic() const { return m_diagnostic; }

private:
    QVariant data(int column, int role) const override;

    const Diagnostic m_diagnostic;
};

class ExplainingStepItem : public Utils::TreeItem
{
public:
    ExplainingStepItem(const ExplainingStep &step);

private:
    QVariant data(int column, int role) const override;

    const ExplainingStep m_step;
};

ClangStaticAnalyzerDiagnosticModel::ClangStaticAnalyzerDiagnosticModel(QObject *parent)
    : Utils::TreeModel(parent)
{
    setHeader(QStringList() << tr("Issue") << tr("Location"));
}

void ClangStaticAnalyzerDiagnosticModel::addDiagnostics(const QList<Diagnostic> &diagnostics)
{
    foreach (const Diagnostic &d, diagnostics)
        rootItem()->appendChild(new DiagnosticItem(d));
}

QList<Diagnostic> ClangStaticAnalyzerDiagnosticModel::diagnostics() const
{
    QList<Diagnostic> diags;
    foreach (const Utils::TreeItem * const item, rootItem()->children())
        diags << static_cast<const DiagnosticItem *>(item)->diagnostic();
    return diags;
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

static QString createExplainingStepToolTipString(const ExplainingStep &step)
{
    if (step.message == step.extendedMessage)
        return createFullLocationString(step.location);

    typedef QPair<QString, QString> StringPair;
    QList<StringPair> lines;

    if (!step.message.isEmpty()) {
        lines << qMakePair(
            QCoreApplication::translate("ClangStaticAnalyzer::ExplainingStep", "Message:"),
                step.message.toHtmlEscaped());
    }
    if (!step.extendedMessage.isEmpty()) {
        lines << qMakePair(
            QCoreApplication::translate("ClangStaticAnalyzer::ExplainingStep", "Extended Message:"),
                step.extendedMessage.toHtmlEscaped());
    }

    lines << qMakePair(
        QCoreApplication::translate("ClangStaticAnalyzer::ExplainingStep", "Location:"),
                createFullLocationString(step.location));

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

static QString createLocationString(const Analyzer::DiagnosticLocation &location)
{
    const QString filePath = location.filePath;
    const QString lineNumber = QString::number(location.line);
    const QString fileAndLine = filePath + QLatin1Char(':') + lineNumber;
    return QLatin1String("in ") + fileAndLine;
}

static QString createExplainingStepNumberString(int number)
{
    const int fieldWidth = 2;
    return QString::fromLatin1("%1:").arg(number, fieldWidth);
}

static QString createExplainingStepString(const ExplainingStep &explainingStep, int number)
{
    return createExplainingStepNumberString(number)
            + QLatin1Char(' ')
            + explainingStep.extendedMessage
            + QLatin1Char(' ')
            + createLocationString(explainingStep.location);
}

static QString fullText(const Diagnostic &diagnostic)
{
    // Summary.
    QString text = diagnostic.category + QLatin1String(": ") + diagnostic.type;
    if (diagnostic.type != diagnostic.description)
        text += QLatin1String(": ") + diagnostic.description;
    text += QLatin1Char('\n');

    // Explaining steps.
    int explainingStepNumber = 1;
    foreach (const ExplainingStep &explainingStep, diagnostic.explainingSteps) {
        text += createExplainingStepString(explainingStep, explainingStepNumber++)
                + QLatin1Char('\n');
    }

    text.chop(1); // Trailing newline.
    return text;
}


DiagnosticItem::DiagnosticItem(const Diagnostic &diag) : m_diagnostic(diag)
{
    // Don't show explaining steps if they add no information.
    if (diag.explainingSteps.count() == 1) {
        const ExplainingStep &step = diag.explainingSteps.first();
        if (step.message == diag.description && step.location == diag.location)
            return;
    }

    foreach (const ExplainingStep &s, diag.explainingSteps)
        appendChild(new ExplainingStepItem(s));
}

QVariant locationData(int role, const Analyzer::DiagnosticLocation &location)
{
    switch (role) {
    case Analyzer::DetailedErrorView::LocationRole:
        return QVariant::fromValue(location);
    case Qt::ToolTipRole:
        return location.filePath.isEmpty() ? QVariant() : QVariant(location.filePath);
    default:
        return QVariant();
    }
}

QVariant DiagnosticItem::data(int column, int role) const
{
    if (column == Analyzer::DetailedErrorView::LocationColumn)
        return locationData(role, m_diagnostic.location);

    // DiagnosticColumn
    switch (role) {
    case Analyzer::DetailedErrorView::FullTextRole:
        return fullText(m_diagnostic);
    case ClangStaticAnalyzerDiagnosticModel::DiagnosticRole:
        return QVariant::fromValue(m_diagnostic);
    case Qt::DisplayRole:
        return m_diagnostic.description;
    case Qt::ToolTipRole:
        return createDiagnosticToolTipString(m_diagnostic);
    default:
        return QVariant();
    }
}

ExplainingStepItem::ExplainingStepItem(const ExplainingStep &step) : m_step(step)
{
}

QVariant ExplainingStepItem::data(int column, int role) const
{
    if (column == Analyzer::DetailedErrorView::LocationColumn)
        return locationData(role, m_step.location);

    // DiagnosticColumn
    switch (role) {
    case Analyzer::DetailedErrorView::FullTextRole:
        return fullText(static_cast<DiagnosticItem *>(parent())->diagnostic());
    case ClangStaticAnalyzerDiagnosticModel::DiagnosticRole:
        return QVariant::fromValue(static_cast<DiagnosticItem *>(parent())->diagnostic());
    case Qt::DisplayRole: {
        const int row = parent()->children().indexOf(const_cast<ExplainingStepItem *>(this)) + 1;
        const int padding = static_cast<int>(std::log10(parent()->rowCount()))
                - static_cast<int>(std::log10(row));
        return QString::fromLatin1("%1%2: %3")
                .arg(QString(padding, QLatin1Char(' ')))
                .arg(row)
                .arg(m_step.message);
    }
    case Qt::ToolTipRole:
        return createExplainingStepToolTipString(m_step);
    default:
        return QVariant();
    }
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
    if (sourceParent.isValid())
        return true;
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
