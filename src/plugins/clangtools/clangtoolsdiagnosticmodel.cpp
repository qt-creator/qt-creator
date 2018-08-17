/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangtoolsdiagnosticmodel.h"

#include "clangtoolsdiagnosticview.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolsutils.h"

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QFileInfo>

#include <cmath>

namespace ClangTools {
namespace Internal {

class ExplainingStepItem : public Utils::TreeItem
{
public:
    ExplainingStepItem(const ExplainingStep &step);

private:
    QVariant data(int column, int role) const override;

    const ExplainingStep m_step;
};

ClangToolsDiagnosticModel::ClangToolsDiagnosticModel(QObject *parent)
    : Utils::TreeModel<>(parent)
{
    setHeader({tr("Issue"), tr("Location"), tr("Fixit Status")});
}

void ClangToolsDiagnosticModel::addDiagnostics(const QList<Diagnostic> &diagnostics)
{
    const auto onFixitStatusChanged = [this](FixitStatus newStatus) {
        if (newStatus == FixitStatus::Scheduled)
            ++m_fixItsToApplyCount;
        else
            --m_fixItsToApplyCount;
        emit fixItsToApplyCountChanged(m_fixItsToApplyCount);
    };

    for (const Diagnostic &d : diagnostics)
        rootItem()->appendChild(new DiagnosticItem(d, onFixitStatusChanged));
}

QList<Diagnostic> ClangToolsDiagnosticModel::diagnostics() const
{
    QList<Diagnostic> diags;
    for (const Utils::TreeItem * const item : *rootItem())
        diags << static_cast<const DiagnosticItem *>(item)->diagnostic();
    return diags;
}

int ClangToolsDiagnosticModel::diagnosticsCount() const
{
    return rootItem()->childCount();
}

static QString createDiagnosticToolTipString(const Diagnostic &diagnostic)
{
    typedef QPair<QString, QString> StringPair;
    QList<StringPair> lines;

    if (!diagnostic.category.isEmpty()) {
        lines << qMakePair(
                     QCoreApplication::translate("ClangTools::Diagnostic", "Category:"),
                     diagnostic.category.toHtmlEscaped());
    }

    if (!diagnostic.type.isEmpty()) {
        lines << qMakePair(
                     QCoreApplication::translate("ClangTools::Diagnostic", "Type:"),
                     diagnostic.type.toHtmlEscaped());
    }

    if (!diagnostic.description.isEmpty()) {
        lines << qMakePair(
                     QCoreApplication::translate("ClangTools::Diagnostic", "Description:"),
                     diagnostic.description.toHtmlEscaped());
    }

    if (!diagnostic.issueContext.isEmpty() && !diagnostic.issueContextKind.isEmpty()) {
        lines << qMakePair(
                     QCoreApplication::translate("ClangTools::Diagnostic", "Context:"),
                     diagnostic.issueContextKind.toHtmlEscaped() + QLatin1Char(' ')
                     + diagnostic.issueContext.toHtmlEscaped());
    }

    lines << qMakePair(
        QCoreApplication::translate("ClangTools::Diagnostic", "Location:"),
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
            QCoreApplication::translate("ClangTools::ExplainingStep", "Message:"),
                step.message.toHtmlEscaped());
    }
    if (!step.extendedMessage.isEmpty()) {
        lines << qMakePair(
            QCoreApplication::translate("ClangTools::ExplainingStep", "Extended message:"),
                step.extendedMessage.toHtmlEscaped());
    }

    lines << qMakePair(
        QCoreApplication::translate("ClangTools::ExplainingStep", "Location:"),
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

static QString createLocationString(const Debugger::DiagnosticLocation &location)
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


DiagnosticItem::DiagnosticItem(const Diagnostic &diag, const OnFixitStatusChanged &onFixitStatusChanged)
    : m_diagnostic(diag)
    , m_onFixitStatusChanged(onFixitStatusChanged)
{
    if (diag.hasFixits)
        m_fixitStatus = FixitStatus::NotScheduled;

    // Don't show explaining steps if they add no information.
    if (diag.explainingSteps.count() == 1) {
        const ExplainingStep &step = diag.explainingSteps.first();
        if (step.message == diag.description && step.location == diag.location)
            return;
    }

    foreach (const ExplainingStep &s, diag.explainingSteps)
        appendChild(new ExplainingStepItem(s));
}

DiagnosticItem::~DiagnosticItem()
{
    setFixitOperations(ReplacementOperations());
}

Qt::ItemFlags DiagnosticItem::flags(int column) const
{
    const Qt::ItemFlags itemFlags = TreeItem::flags(column);
    if (column == DiagnosticView::FixItColumn) {
        switch (m_fixitStatus) {
        case FixitStatus::NotAvailable:
        case FixitStatus::Applied:
        case FixitStatus::FailedToApply:
        case FixitStatus::Invalidated:
            return itemFlags & ~Qt::ItemIsEnabled;
        case FixitStatus::Scheduled:
        case FixitStatus::NotScheduled:
            return itemFlags | Qt::ItemIsUserCheckable;
        }
    }

    return itemFlags;
}

static QVariant iconData(const QString &type)
{
    if (type == "warning")
        return Utils::Icons::CODEMODEL_WARNING.icon();
    if (type == "error" || type == "fatal")
        return Utils::Icons::CODEMODEL_ERROR.icon();
    if (type == "note")
        return Utils::Icons::BOOKMARK.icon();
    if (type == "fix-it")
        return Utils::Icons::CODEMODEL_FIXIT.icon();
    return QVariant();
}

QVariant DiagnosticItem::data(int column, int role) const
{
    if (column == Debugger::DetailedErrorView::LocationColumn)
        return Debugger::DetailedErrorView::locationData(role, m_diagnostic.location);

    if (column == DiagnosticView::FixItColumn) {
        if (role == Qt::CheckStateRole) {
            switch (m_fixitStatus) {
            case FixitStatus::NotAvailable:
            case FixitStatus::NotScheduled:
            case FixitStatus::Invalidated:
            case FixitStatus::Applied:
            case FixitStatus::FailedToApply:
                return Qt::Unchecked;
            case FixitStatus::Scheduled:
                return Qt::Checked;
            }
        } else if (role == Qt::DisplayRole) {
            switch (m_fixitStatus) {
            case FixitStatus::NotAvailable:
                return ClangToolsDiagnosticModel::tr("No Fixits");
            case FixitStatus::NotScheduled:
                return ClangToolsDiagnosticModel::tr("Not Scheduled");
            case FixitStatus::Invalidated:
                return ClangToolsDiagnosticModel::tr("Invalidated");
            case FixitStatus::Scheduled:
                return ClangToolsDiagnosticModel::tr("Scheduled");
            case FixitStatus::FailedToApply:
                return ClangToolsDiagnosticModel::tr("Failed to Apply");
            case FixitStatus::Applied:
                return ClangToolsDiagnosticModel::tr("Applied");
            }
        }
        return QVariant();
    }

    // DiagnosticColumn
    switch (role) {
    case Debugger::DetailedErrorView::FullTextRole:
        return fullText(m_diagnostic);
    case ClangToolsDiagnosticModel::DiagnosticRole:
        return QVariant::fromValue(m_diagnostic);
    case Qt::DisplayRole:
        return m_diagnostic.description;
    case Qt::ToolTipRole:
        return createDiagnosticToolTipString(m_diagnostic);
    case Qt::DecorationRole:
        return iconData(m_diagnostic.type);
    default:
        return QVariant();
    }
}

bool DiagnosticItem::setData(int column, const QVariant &data, int role)
{
    if (column == DiagnosticView::FixItColumn && role == Qt::CheckStateRole) {
        if (m_fixitStatus != FixitStatus::Scheduled && m_fixitStatus != FixitStatus::NotScheduled)
            return false;

        const FixitStatus newStatus = data.value<Qt::CheckState>() == Qt::Checked
                                          ? FixitStatus::Scheduled
                                          : FixitStatus::NotScheduled;

        setFixItStatus(newStatus);
        return true;
    }

    return Utils::TreeItem::setData(column, data, role);
}

void DiagnosticItem::setFixItStatus(const FixitStatus &status)
{
    const FixitStatus oldStatus = m_fixitStatus;
    m_fixitStatus = status;
    update();
    if (m_onFixitStatusChanged && status != oldStatus)
        m_onFixitStatusChanged(status);
}

void DiagnosticItem::setFixitOperations(const ReplacementOperations &replacements)
{
    qDeleteAll(m_fixitOperations);
    m_fixitOperations = replacements;
}

ExplainingStepItem::ExplainingStepItem(const ExplainingStep &step) : m_step(step)
{
}

QVariant ExplainingStepItem::data(int column, int role) const
{
    if (column == Debugger::DetailedErrorView::LocationColumn)
        return Debugger::DetailedErrorView::locationData(role, m_step.location);

    if (column == DiagnosticView::FixItColumn)
        return QVariant();

    // DiagnosticColumn
    switch (role) {
    case Debugger::DetailedErrorView::FullTextRole:
        return fullText(static_cast<DiagnosticItem *>(parent())->diagnostic());
    case ClangToolsDiagnosticModel::DiagnosticRole:
        return QVariant::fromValue(static_cast<DiagnosticItem *>(parent())->diagnostic());
    case Qt::DisplayRole: {
        const int row = indexInParent() + 1;
        const int padding = static_cast<int>(std::log10(parent()->childCount()))
                - static_cast<int>(std::log10(row));
        return QString::fromLatin1("%1%2: %3")
                .arg(QString(padding, QLatin1Char(' ')))
                .arg(row)
                .arg(m_step.message);
    }
    case Qt::ToolTipRole:
        return createExplainingStepToolTipString(m_step);
    case Qt::DecorationRole:
        return (m_step.message.startsWith("fix-it:")) ? iconData("fix-it") : QVariant();
    default:
        return QVariant();
    }
}


DiagnosticFilterModel::DiagnosticFilterModel(QObject *parent)
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

void DiagnosticFilterModel::setProject(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    if (m_project) {
        disconnect(ClangToolsProjectSettingsManager::getSettings(m_project),
                   &ClangToolsProjectSettings::suppressedDiagnosticsChanged, this,
                   &DiagnosticFilterModel::handleSuppressedDiagnosticsChanged);
    }
    m_project = project;
    m_lastProjectDirectory = m_project->projectDirectory();
    connect(ClangToolsProjectSettingsManager::getSettings(m_project),
            &ClangToolsProjectSettings::suppressedDiagnosticsChanged,
            this, &DiagnosticFilterModel::handleSuppressedDiagnosticsChanged);
    handleSuppressedDiagnosticsChanged();
}

void DiagnosticFilterModel::addSuppressedDiagnostic(
        const SuppressedDiagnostic &diag)
{
    QTC_ASSERT(!m_project, return);
    m_suppressedDiagnostics << diag;
    invalidate();
}

bool DiagnosticFilterModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
    // Avoid filtering child diagnostics / explaining steps.
    if (sourceParent.isValid())
        return true;

    // Is the diagnostic suppressed?
    auto model = static_cast<ClangToolsDiagnosticModel *>(sourceModel());
    auto item = static_cast<DiagnosticItem *>(model->rootItem()->childAt(sourceRow));
    const Diagnostic &diag = item->diagnostic();
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

    // Does the diagnostic match the filter?
    if (diag.description.contains(filterRegExp()))
        return true;

    return false;
}

void DiagnosticFilterModel::handleSuppressedDiagnosticsChanged()
{
    QTC_ASSERT(m_project, return);
    m_suppressedDiagnostics
            = ClangToolsProjectSettingsManager::getSettings(m_project)->suppressedDiagnostics();
    invalidate();
}

} // namespace Internal
} // namespace ClangTools
