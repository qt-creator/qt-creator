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

#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QFileInfo>
#include <QLoggingCategory>

#include <tuple>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.model", QtWarningMsg)

namespace ClangTools {
namespace Internal {

FilePathItem::FilePathItem(const QString &filePath)
    : m_filePath(filePath)
{}

QVariant FilePathItem::data(int column, int role) const
{
    if (column == DiagnosticView::DiagnosticColumn) {
        switch (role) {
        case Qt::DisplayRole:
            return m_filePath;
        case Qt::DecorationRole:
            return Core::FileIconProvider::icon(m_filePath);
        case Debugger::DetailedErrorView::FullTextRole:
            return m_filePath;
        default:
            return QVariant();
        }
    }

    return QVariant();
}

class ExplainingStepItem : public Utils::TreeItem
{
public:
    ExplainingStepItem(const ExplainingStep &step, int index);
    int index() const { return m_index; }

private:
    QVariant data(int column, int role) const override;

    const ExplainingStep m_step;
    const int m_index = 0;
};

ClangToolsDiagnosticModel::ClangToolsDiagnosticModel(QObject *parent)
    : ClangToolsDiagnosticModelBase(parent)
    , m_filesWatcher(std::make_unique<QFileSystemWatcher>())
{
    setHeader({tr("Diagnostic")});
    connectFileWatcher();
}

QDebug operator<<(QDebug debug, const Diagnostic &d)
{
    return debug << "category:" << d.category
                 << "type:" << d.type
                 << "context:" << d.issueContext
                 << "contextKind:" << d.issueContextKind
                 << "hasFixits:" << d.hasFixits
                 << "explainingSteps:" << d.explainingSteps.size()
                 << "location:" << d.location
                 << "description:" << d.description
                 ;
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

    for (const Diagnostic &d : diagnostics) {
        // Check for duplicates
        const int previousItemCount = m_diagnostics.count();
        m_diagnostics.insert(d);
        if (m_diagnostics.count() == previousItemCount) {
            qCDebug(LOG) << "Not adding duplicate diagnostic:" << d;
            continue;
        }

        // Create file path item if necessary
        const QString filePath = d.location.filePath;
        FilePathItem *&filePathItem = m_filePathToItem[filePath];
        if (!filePathItem) {
            filePathItem = new FilePathItem(filePath);
            rootItem()->appendChild(filePathItem);

            addWatchedPath(d.location.filePath);
        }

        // Add to file path item
        qCDebug(LOG) << "Adding diagnostic:" << d;
        filePathItem->appendChild(new DiagnosticItem(d, onFixitStatusChanged, this));
    }
}

QSet<Diagnostic> ClangToolsDiagnosticModel::diagnostics() const
{
    return m_diagnostics;
}

void ClangToolsDiagnosticModel::clear()
{
    m_filePathToItem.clear();
    m_diagnostics.clear();
    clearAndSetupCache();
    ClangToolsDiagnosticModelBase::clear();
}

void ClangToolsDiagnosticModel::updateItems(const DiagnosticItem *changedItem)
{
    for (auto item : stepsToItemsCache[changedItem->diagnostic().explainingSteps]) {
        if (item != changedItem)
            item->setFixItStatus(changedItem->fixItStatus());
    }
}

void ClangToolsDiagnosticModel::connectFileWatcher()
{
    connect(m_filesWatcher.get(),
            &QFileSystemWatcher::fileChanged,
            this,
            &ClangToolsDiagnosticModel::onFileChanged);
}

void ClangToolsDiagnosticModel::clearAndSetupCache()
{
    m_filesWatcher = std::make_unique<QFileSystemWatcher>();
    connectFileWatcher();
    stepsToItemsCache.clear();
}

void ClangToolsDiagnosticModel::onFileChanged(const QString &path)
{
    forItemsAtLevel<2>([&](DiagnosticItem *item){
        if (item->diagnostic().location.filePath == path)
            item->setFixItStatus(FixitStatus::Invalidated);
    });
    removeWatchedPath(path);
}

void ClangToolsDiagnosticModel::removeWatchedPath(const QString &path)
{
    m_filesWatcher->removePath(path);
}

void ClangToolsDiagnosticModel::addWatchedPath(const QString &path)
{
    m_filesWatcher->addPath(path);
}

static QString fixitStatus(FixitStatus status)
{
    switch (status) {
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
    return QString();
}

static QString createDiagnosticToolTipString(const Diagnostic &diagnostic, FixitStatus fixItStatus)
{
    using StringPair = QPair<QString, QString>;
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

    lines << qMakePair(
        QCoreApplication::translate("ClangTools::Diagnostic", "Fixit status:"),
        fixitStatus(fixItStatus));

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

    using StringPair = QPair<QString, QString>;
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
            + explainingStep.message
            + QLatin1Char(' ')
            + createLocationString(explainingStep.location);
}


static QString lineColumnString(const Debugger::DiagnosticLocation &location)
{
    return QString("%1:%2").arg(QString::number(location.line), QString::number(location.column));
}

static QString fullText(const Diagnostic &diagnostic)
{
    QString text = diagnostic.location.filePath + QLatin1Char(':');
    text += lineColumnString(diagnostic.location) + QLatin1String(": ");
    if (!diagnostic.category.isEmpty())
        text += diagnostic.category + QLatin1String(": ");
    text += diagnostic.type;
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

DiagnosticItem::DiagnosticItem(const Diagnostic &diag,
                               const OnFixitStatusChanged &onFixitStatusChanged,
                               ClangToolsDiagnosticModel *parent)
    : m_diagnostic(diag)
    , m_onFixitStatusChanged(onFixitStatusChanged)
    , m_parentModel(parent)
{
    if (diag.hasFixits)
        m_fixitStatus = FixitStatus::NotScheduled;

    // Don't show explaining steps if they add no information.
    if (diag.explainingSteps.count() == 1) {
        const ExplainingStep &step = diag.explainingSteps.first();
        if (step.message == diag.description && step.location == diag.location)
            return;
    }

    if (!diag.explainingSteps.isEmpty())
        m_parentModel->stepsToItemsCache[diag.explainingSteps].push_back(this);

    for (int i = 0; i < diag.explainingSteps.size(); ++i )
        appendChild(new ExplainingStepItem(diag.explainingSteps[i], i));
}

DiagnosticItem::~DiagnosticItem()
{
    setFixitOperations(ReplacementOperations());
}

Qt::ItemFlags DiagnosticItem::flags(int column) const
{
    const Qt::ItemFlags itemFlags = TreeItem::flags(column);
    if (column == DiagnosticView::DiagnosticColumn)
        return itemFlags | Qt::ItemIsUserCheckable;
    return itemFlags;
}

static QVariant iconData(const QString &type)
{
    if (type == "warning")
        return Utils::Icons::CODEMODEL_WARNING.icon();
    if (type == "error" || type == "fatal")
        return Utils::Icons::CODEMODEL_ERROR.icon();
    if (type == "note")
        return Utils::Icons::INFO.icon();
    if (type == "fix-it")
        return Utils::Icons::CODEMODEL_FIXIT.icon();
    return QVariant();
}

QVariant DiagnosticItem::data(int column, int role) const
{
    if (column == DiagnosticView::DiagnosticColumn) {
        switch (role) {
        case Debugger::DetailedErrorView::LocationRole:
            return QVariant::fromValue(m_diagnostic.location);
        case Debugger::DetailedErrorView::FullTextRole:
            return fullText(m_diagnostic);
        case ClangToolsDiagnosticModel::DiagnosticRole:
            return QVariant::fromValue(m_diagnostic);
        case ClangToolsDiagnosticModel::TextRole:
            return m_diagnostic.description;
        case ClangToolsDiagnosticModel::CheckBoxEnabledRole:
            switch (m_fixitStatus) {
            case FixitStatus::NotAvailable:
            case FixitStatus::Applied:
            case FixitStatus::FailedToApply:
            case FixitStatus::Invalidated:
                return false;
            case FixitStatus::Scheduled:
            case FixitStatus::NotScheduled:
                return true;
            }
            break;
        case Qt::CheckStateRole: {
            switch (m_fixitStatus) {
            case FixitStatus::NotAvailable:
            case FixitStatus::Invalidated:
            case FixitStatus::Applied:
            case FixitStatus::FailedToApply:
            case FixitStatus::NotScheduled:
                return Qt::Unchecked;
            case FixitStatus::Scheduled:
                return Qt::Checked;
            }
            break;
        }
        case Qt::DisplayRole:
            return QString("%1: %2").arg(lineColumnString(m_diagnostic.location),
                                         m_diagnostic.description);
        case Qt::ToolTipRole:
            return createDiagnosticToolTipString(m_diagnostic, m_fixitStatus);
        case Qt::DecorationRole:
            return iconData(m_diagnostic.type);
        default:
            return QVariant();
        }
    }

    return QVariant();
}

bool DiagnosticItem::setData(int column, const QVariant &data, int role)
{
    if (column == DiagnosticView::DiagnosticColumn && role == Qt::CheckStateRole) {
        if (m_fixitStatus != FixitStatus::Scheduled && m_fixitStatus != FixitStatus::NotScheduled)
            return false;

        const FixitStatus newStatus = data.value<Qt::CheckState>() == Qt::Checked
                                          ? FixitStatus::Scheduled
                                          : FixitStatus::NotScheduled;

        setFixItStatus(newStatus);
        m_parentModel->updateItems(this);
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

bool DiagnosticItem::hasNewFixIts() const
{
    if (m_diagnostic.explainingSteps.empty())
        return false;

    return m_parentModel->stepsToItemsCache[m_diagnostic.explainingSteps].front() == this;
}

ExplainingStepItem::ExplainingStepItem(const ExplainingStep &step, int index)
    : m_step(step)
    , m_index(index)
{}

static QString rangeString(const QVector<Debugger::DiagnosticLocation> &ranges)
{
    return QString("%1-%2").arg(lineColumnString(ranges[0]), lineColumnString(ranges[1]));
}

QVariant ExplainingStepItem::data(int column, int role) const
{
    if (column == DiagnosticView::DiagnosticColumn) {
        // DiagnosticColumn
        switch (role) {
        case Debugger::DetailedErrorView::LocationRole:
            return QVariant::fromValue(m_step.location);
        case Debugger::DetailedErrorView::FullTextRole: {
            return QString("%1:%2: %3")
                .arg(m_step.location.filePath, lineColumnString(m_step.location), m_step.message);
        }
        case ClangToolsDiagnosticModel::TextRole:
            return m_step.message;
        case ClangToolsDiagnosticModel::DiagnosticRole:
            return QVariant::fromValue(static_cast<DiagnosticItem *>(parent())->diagnostic());
        case Qt::DisplayRole: {
            const QString mainFilePath = static_cast<DiagnosticItem *>(parent())->diagnostic().location.filePath;
            const QString locationString
                = m_step.location.filePath == mainFilePath
                      ? lineColumnString(m_step.location)
                      : QString("%1:%2").arg(QFileInfo(m_step.location.filePath).fileName(),
                                             lineColumnString(m_step.location));

            if (m_step.isFixIt) {
                if (m_step.ranges[0] == m_step.ranges[1]) {
                    return QString("%1: Insertion of \"%2\".")
                        .arg(locationString, m_step.message);
                }
                if (m_step.message.isEmpty()) {
                    return QString("%1: Removal of %2.")
                        .arg(locationString, rangeString(m_step.ranges));
                }
                return QString("%1: Replacement of %2 with: \"%3\".")
                    .arg(locationString,
                         rangeString(m_step.ranges),
                         m_step.message);
            }
            return QString("%1: %2").arg(locationString, m_step.message);
        }
        case Qt::ToolTipRole:
            return createExplainingStepToolTipString(m_step);
        case Qt::DecorationRole:
            if (m_step.isFixIt)
                return Utils::Icons::CODEMODEL_FIXIT.icon();
            return Utils::Icons::INFO.icon();
        default:
            return QVariant();
        }
    }

    return QVariant();
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

void DiagnosticFilterModel::invalidateFilter()
{
    QSortFilterProxyModel::invalidateFilter();
}

bool DiagnosticFilterModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
    auto model = static_cast<ClangToolsDiagnosticModel *>(sourceModel());

    // FilePathItem - hide if no diagnostics match
    if (!sourceParent.isValid()) {
        const QModelIndex filePathIndex = model->index(sourceRow, 0);
        const int rowCount = model->rowCount(filePathIndex);
        if (rowCount == 0)
            return true; // Children not yet added.
        for (int row = 0; row < rowCount; ++row) {
            if (filterAcceptsRow(row, filePathIndex))
                return true;
        }
        return false;
    }

    // DiagnosticItem
    Utils::TreeItem *parentItem = model->itemForIndex(sourceParent);
    QTC_ASSERT(parentItem, return true);
    if (parentItem->level() == 1) {
        auto filePathItem = static_cast<FilePathItem *>(parentItem);
        auto diagnosticItem = static_cast<DiagnosticItem *>(filePathItem->childAt(sourceRow));

        // Is the diagnostic explicitly suppressed?
        const Diagnostic &diag = diagnosticItem->diagnostic();
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
        return diag.description.contains(filterRegExp());
    }

    return true; // ExplainingStepItem
}

bool DiagnosticFilterModel::lessThan(const QModelIndex &l, const QModelIndex &r) const
{
    auto model = static_cast<ClangToolsDiagnosticModel *>(sourceModel());
    Utils::TreeItem *itemLeft = model->itemForIndex(l);
    QTC_ASSERT(itemLeft, return QSortFilterProxyModel::lessThan(l, r));
    const bool isComparingDiagnostics = itemLeft->level() > 1;

    if (sortColumn() == Debugger::DetailedErrorView::DiagnosticColumn && isComparingDiagnostics) {
        bool result = false;
        if (itemLeft->level() == 2) {
            using Debugger::DiagnosticLocation;
            const int role = Debugger::DetailedErrorView::LocationRole;

            const auto leftLoc = sourceModel()->data(l, role).value<DiagnosticLocation>();
            const auto leftText
                = sourceModel()->data(l, ClangToolsDiagnosticModel::TextRole).toString();

            const auto rightLoc = sourceModel()->data(r, role).value<DiagnosticLocation>();
            const auto rightText
                = sourceModel()->data(r, ClangToolsDiagnosticModel::TextRole).toString();

            result = std::tie(leftLoc.line, leftLoc.column, leftText)
                     < std::tie(rightLoc.line, rightLoc.column, rightText);
        } else if (itemLeft->level() == 3) {
            Utils::TreeItem *itemRight = model->itemForIndex(r);
            QTC_ASSERT(itemRight, QSortFilterProxyModel::lessThan(l, r));
            const auto left = static_cast<ExplainingStepItem *>(itemLeft);
            const auto right = static_cast<ExplainingStepItem *>(itemRight);
            result = left->index() < right->index();
        } else {
            QTC_CHECK(false && "Unexpected item");
        }

        if (sortOrder() == Qt::DescendingOrder)
            return !result; // Do not change the order of these item as this might be confusing.
        return result;
    }

    // FilePathItem
    return QSortFilterProxyModel::lessThan(l, r);
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
