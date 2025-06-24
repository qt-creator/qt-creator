// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsdiagnosticmodel.h"

#include "clangtoolsdiagnosticview.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolstr.h"
#include "clangtoolsutils.h"
#include "diagnosticmark.h"
#include "inlinesuppresseddiagnostics.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/textmark.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QLoggingCategory>

#include <tuple>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.model", QtWarningMsg)

using namespace Utils;

namespace ClangTools {
namespace Internal {

FilePathItem::FilePathItem(const Utils::FilePath &filePath)
    : m_filePath(filePath)
{}

QVariant FilePathItem::data(int column, int role) const
{
    if (column == DiagnosticView::DiagnosticColumn) {
        switch (role) {
        case Qt::DisplayRole:
            return m_filePath.toUserOutput();
        case Qt::DecorationRole:
            return Utils::FileIconProvider::icon(m_filePath);
        case Debugger::DetailedErrorView::FullTextRole:
            return m_filePath.toUserOutput();
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

ClangToolsDiagnosticModel::ClangToolsDiagnosticModel(CppEditor::ClangToolType type, QObject *parent)
    : ClangToolsDiagnosticModelBase(parent)
    , m_filesWatcher(std::make_unique<Utils::FileSystemWatcher>())
    , m_type(type)
{
    setRootItem(createRootItem());
    connectFileWatcher();
}

QDebug operator<<(QDebug debug, const Diagnostic &d)
{
    return debug << "name:" << d.name
                 << "category:" << d.category
                 << "type:" << d.type
                 << "hasFixits:" << d.hasFixits
                 << "explainingSteps:" << d.explainingSteps.size()
                 << "location:" << d.location
                 << "description:" << d.description
                 ;
}

void ClangToolsDiagnosticModel::addDiagnostics(
    const Diagnostics &diagnostics, bool generateMarks, TreeItem *rootItem)
{
    for (const Diagnostic &d : diagnostics) {
        // Check for duplicates
        const int previousItemCount = m_diagnostics.count();
        m_diagnostics.insert(d);
        if (m_diagnostics.count() == previousItemCount) {
            qCDebug(LOG) << "Not adding duplicate diagnostic:" << d;
            continue;
        }

        // Create file path item if necessary
        const FilePath &filePath = d.location.targetFilePath;
        FilePathItem *&filePathItem = m_filePathToItem[filePath];
        if (!filePathItem) {
            filePathItem = new FilePathItem(filePath);
            rootItem->appendChild(filePathItem);
            addWatchedPath(filePath);
        }

        // Add to file path item
        qCDebug(LOG) << "Adding diagnostic:" << d;
        filePathItem->appendChild(new DiagnosticItem(d, generateMarks, this));
    }
}

QSet<Diagnostic> ClangToolsDiagnosticModel::diagnostics() const
{
    return m_diagnostics;
}

QSet<QString> ClangToolsDiagnosticModel::allChecks() const
{
    QSet<QString> checks;
    forItemsAtLevel<2>([&](DiagnosticItem *item) {
        checks.insert(item->diagnostic().name);
    });

    return checks;
}

void ClangToolsDiagnosticModel::clear()
{
    beginResetModel();
    m_filePathToItem.clear();
    m_diagnostics.clear();
    clearAndSetupCache();
    ClangToolsDiagnosticModelBase::clear();
    endResetModel();
}

void ClangToolsDiagnosticModel::connectFileWatcher()
{
    connect(m_filesWatcher.get(),
            &Utils::FileSystemWatcher::fileChanged,
            this,
            &ClangToolsDiagnosticModel::onFileChanged);
}

void ClangToolsDiagnosticModel::clearAndSetupCache()
{
    m_filesWatcher = std::make_unique<Utils::FileSystemWatcher>();
    connectFileWatcher();
    m_stepsToItemsCache.clear();
}

void ClangToolsDiagnosticModel::onFileChanged(const FilePath &path)
{
    forItemsAtLevel<2>([&](DiagnosticItem *item){
        if (item->diagnostic().location.targetFilePath == path)
            item->setFixItStatus(FixitStatus::Invalidated, true);
    });
    m_filesWatcher->removeFile(path);
}

void ClangToolsDiagnosticModel::removeWatchedPath(const Utils::FilePath &path)
{
    m_filesWatcher->removeFile(path);
}

void ClangToolsDiagnosticModel::addWatchedPath(const Utils::FilePath &path)
{
    m_filesWatcher->addFile(path, Utils::FileSystemWatcher::WatchAllChanges);
}

TreeItem *ClangToolsDiagnosticModel::createRootItem() const
{
    return new StaticTreeItem(QString());
}

std::unique_ptr<InlineSuppressedDiagnostics> ClangToolsDiagnosticModel::createInlineSuppressedDiagnostics()
{
    switch (m_type) {
    case CppEditor::ClangToolType::Tidy:
        return std::make_unique<InlineSuppressedClangTidyDiagnostics>();
    case CppEditor::ClangToolType::Clazy:
        return std::make_unique<InlineSuppressedClazyDiagnostics>();
    }
    QTC_ASSERT(false, return {});
}

const QList<DiagnosticItem *> &ClangToolsDiagnosticModel::itemsWithSameFixits(
    const DiagnosticItem *item)
{
    return m_stepsToItemsCache[item->diagnostic().explainingSteps];
}

static QString lineColumnString(const Link &location)
{
    return QString("%1:%2").arg(location.target.line).arg(location.target.column + 1);
}

static QString createExplainingStepToolTipString(const ExplainingStep &step)
{
    using StringPair = QPair<QString, QString>;
    QList<StringPair> lines;

    if (!step.message.isEmpty()) {
        lines.push_back({Tr::tr("Message:"),
                         step.message.toHtmlEscaped()});
    }

    lines.push_back({Tr::tr("Location:"),
                     createFullLocationString(step.location)});

    QString html = QLatin1String("<html>"
                   "<head>"
                   "<style>dt { font-weight:bold; } dd { font-family: monospace; }</style>\n"
                   "<body><dl>");

    for (const StringPair &pair : std::as_const(lines)) {
        html += QLatin1String("<dt>");
        html += pair.first;
        html += QLatin1String("</dt><dd>");
        html += pair.second;
        html += QLatin1String("</dd>\n");
    }
    html += QLatin1String("</dl></body></html>");
    return html;
}

static QString createLocationString(const Link &location)
{
    const QString filePath = location.targetFilePath.toUserOutput();
    const QString lineNumber = QString::number(location.target.line);
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


static QString fullText(const Diagnostic &diagnostic)
{
    QString text = diagnostic.location.targetFilePath.toUserOutput() + QLatin1Char(':');
    text += lineColumnString(diagnostic.location) + QLatin1String(": ");
    if (!diagnostic.category.isEmpty())
        text += diagnostic.category + QLatin1String(": ");
    text += diagnostic.type;
    if (diagnostic.type != diagnostic.description)
        text += QLatin1String(": ") + diagnostic.description;
    text += QLatin1Char('\n');

    // Explaining steps.
    int explainingStepNumber = 1;
    for (const ExplainingStep &explainingStep : std::as_const(diagnostic.explainingSteps)) {
        text += createExplainingStepString(explainingStep, explainingStepNumber++)
                + QLatin1Char('\n');
    }

    text.chop(1); // Trailing newline.
    return text;
}

DiagnosticItem::DiagnosticItem(const Diagnostic &diag,
                               bool generateMark,
                               ClangToolsDiagnosticModel *model)
    : m_diagnostic(diag)
    , m_mark(generateMark ? new DiagnosticMark(diag) : nullptr)
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
        model->m_stepsToItemsCache[diag.explainingSteps].push_back(this);

    for (int i = 0; i < diag.explainingSteps.size(); ++i )
        appendChild(new ExplainingStepItem(diag.explainingSteps[i], i));
}

DiagnosticItem::~DiagnosticItem()
{
    delete m_mark;
}

void DiagnosticItem::setTextMarkVisible(bool visible)
{
    if (m_mark)
        m_mark->setVisible(visible);
}

Qt::ItemFlags DiagnosticItem::flags(int column) const
{
    const Qt::ItemFlags itemFlags = TreeItem::flags(column);
    if (column == DiagnosticView::DiagnosticColumn)
        return itemFlags | Qt::ItemIsUserCheckable;
    return itemFlags;
}

static QVariant iconData(const Diagnostic &diagnostic)
{
    QIcon icon = diagnostic.icon();
    return icon.isNull() ? QVariant() : QVariant(icon);
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
        case ClangToolsDiagnosticModel::InlineSuppressableRole:
            switch (m_fixitStatus) {
            case FixitStatus::Applied:
            case FixitStatus::FailedToApply:
            case FixitStatus::Invalidated:
                return false;
            case FixitStatus::Scheduled:
            case FixitStatus::NotAvailable:
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
        case ClangToolsDiagnosticModel::DocumentationUrlRole:
            return documentationUrl(m_diagnostic.name);
        case Qt::DisplayRole:
            return QString("%1: %2").arg(lineColumnString(m_diagnostic.location),
                                         m_diagnostic.description);
        case Qt::ToolTipRole:
            return createDiagnosticToolTipString(m_diagnostic, m_fixitStatus, false);
        case Qt::DecorationRole:
            return iconData(m_diagnostic);
        default:
            return QVariant();
        }
    }

    return QVariant();
}

ClangToolsDiagnosticModel *DiagnosticItem::diagModel() const
{
    return qobject_cast<ClangToolsDiagnosticModel *>(model());
}

bool DiagnosticItem::setData(int column, const QVariant &data, int role)
{
    if (column == DiagnosticView::DiagnosticColumn && role == Qt::CheckStateRole) {
        const FixitStatus newStatus = data.value<Qt::CheckState>() == Qt::Checked
            ? FixitStatus::Scheduled
            : FixitStatus::NotScheduled;
        if (scheduleOrUnscheduleFixit(newStatus, true)) {
            for (auto item : diagModel()->itemsWithSameFixits(this)) {
                if (item != this)
                    item->setFixItStatus(newStatus, true);
            }
        }
        return false; // We already called update().
    }

    return Utils::TreeItem::setData(column, data, role);
}

void DiagnosticItem::setFixItStatus(const FixitStatus &status, bool updateUi)
{
    const FixitStatus oldStatus = m_fixitStatus;
    if (oldStatus == status)
        return;
    m_fixitStatus = status;
    updateColumn(DiagnosticView::DiagnosticColumn);
    emit diagModel()->fixitStatusChanged(this, oldStatus, status, updateUi);
    if (status == FixitStatus::Applied || status == FixitStatus::Invalidated) {
        delete m_mark;
        m_mark = nullptr;
    }
}

bool DiagnosticItem::scheduleOrUnscheduleFixit(FixitStatus status, bool updateUi)
{
    QTC_ASSERT(status == FixitStatus::Scheduled || status == FixitStatus::NotScheduled, return false);
    if (m_fixitStatus == FixitStatus::Scheduled || m_fixitStatus == FixitStatus::NotScheduled) {
        setFixItStatus(status, updateUi);
        return true;
    }
    return false;
}

bool DiagnosticItem::hasNewFixIts() const
{
    if (m_diagnostic.explainingSteps.empty())
        return false;

    return diagModel()->itemsWithSameFixits(this).front() == this;
}

ExplainingStepItem::ExplainingStepItem(const ExplainingStep &step, int index)
    : m_step(step)
    , m_index(index)
{}

static QString rangeString(const Links &ranges)
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
                .arg(m_step.location.targetFilePath.toUserOutput(),
                     lineColumnString(m_step.location),
                     m_step.message);
        }
        case ClangToolsDiagnosticModel::TextRole:
            return m_step.message;
        case ClangToolsDiagnosticModel::DiagnosticRole:
            return QVariant::fromValue(static_cast<DiagnosticItem *>(parent())->diagnostic());
        case ClangToolsDiagnosticModel::DocumentationUrlRole:
            return parent()->data(column, role);
        case Qt::DisplayRole: {
            const Utils::FilePath mainFilePath
                = static_cast<DiagnosticItem *>(parent())->diagnostic().location.targetFilePath;
            const QString locationString
                = m_step.location.targetFilePath == mainFilePath
                      ? lineColumnString(m_step.location)
                      : QString("%1:%2").arg(m_step.location.targetFilePath.fileName(),
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
    connect(ProjectExplorer::ProjectManager::instance(),
            &ProjectExplorer::ProjectManager::projectAdded, this,
            [this](ProjectExplorer::Project *project) {
                if (!m_project && project->projectDirectory() == m_lastProjectDirectory)
                    setProject(project);
            });
    connect(this, &QAbstractItemModel::modelReset, this, [this] {
        reset();
        const Counters counters = countDiagnostics(QModelIndex(), 0, rowCount());
        m_diagnostics = counters.diagnostics;
        m_fixitsSchedulable = counters.fixits;
        emit fixitCountersChanged();
    });
    connect(this, &QAbstractItemModel::rowsInserted,
            this, [this](const QModelIndex &parent, int first, int last) {
        const Counters counters = countDiagnostics(parent, first, last);
        m_diagnostics += counters.diagnostics;
        m_fixitsSchedulable += counters.fixits;
        emit fixitCountersChanged();
    });
    connect(this, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, [this](const QModelIndex &parent, int first, int last) {
        const Counters counters = countDiagnostics(parent, first, last);
        m_diagnostics -= counters.diagnostics;
        m_fixitsSchedulable -= counters.fixits;
        emit fixitCountersChanged();
    });
}

void DiagnosticFilterModel::setProject(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    if (m_project) {
        disconnect(ClangToolsProjectSettings::getSettings(m_project).get(),
                   &ClangToolsProjectSettings::suppressedDiagnosticsChanged, this,
                   &DiagnosticFilterModel::handleSuppressedDiagnosticsChanged);
    }
    m_project = project;
    m_lastProjectDirectory = m_project->projectDirectory();
    connect(ClangToolsProjectSettings::getSettings(m_project).get(),
            &ClangToolsProjectSettings::suppressedDiagnosticsChanged,
            this, &DiagnosticFilterModel::handleSuppressedDiagnosticsChanged);
    handleSuppressedDiagnosticsChanged();
}

void DiagnosticFilterModel::addSuppressedDiagnostics(const SuppressedDiagnosticsList &diags)
{
    m_suppressedDiagnostics << diags;
    invalidate();
}

void DiagnosticFilterModel::addSuppressedDiagnostic(const SuppressedDiagnostic &diag)
{
    QTC_ASSERT(!m_project, return);
    m_suppressedDiagnostics << diag;
    invalidate();
}

void DiagnosticFilterModel::onFixitStatusChanged(const DiagnosticItem *item,
                                                 FixitStatus oldStatus,
                                                 FixitStatus newStatus,
                                                 bool updateUi)
{
    if (!filterAcceptsItem(item))
        return;

    if (newStatus == FixitStatus::Scheduled)
        ++m_fixitsScheduled;
    else if (oldStatus == FixitStatus::Scheduled) {
        --m_fixitsScheduled;
        if (newStatus != FixitStatus::NotScheduled)
            --m_fixitsSchedulable;
    }

    if (updateUi)
        emit fixitCountersChanged();
}

void DiagnosticFilterModel::reset()
{
    m_filterOptions.reset();

    m_fixitsScheduled = 0;
    m_fixitsSchedulable = 0;
    m_diagnostics = 0;
}

DiagnosticFilterModel::Counters DiagnosticFilterModel::countDiagnostics(const QModelIndex &parent,
                                                                        int first,
                                                                        int last) const
{
    Counters counters;
    const auto countItem = [&](Utils::TreeItem *item){
        if (!mapFromSource(item->index()).isValid())
            return; // Do not count filtered out items.
        ++counters.diagnostics;
        if (static_cast<DiagnosticItem *>(item)->diagnostic().hasFixits)
            ++counters.fixits;
    };

    auto model = static_cast<ClangToolsDiagnosticModel *>(sourceModel());
    for (int row = first; row <= last; ++row) {
        Utils::TreeItem *treeItem = model->itemForIndex(mapToSource(index(row, 0, parent)));
        if (treeItem->level() == 1)
            static_cast<FilePathItem *>(treeItem)->forChildrenAtLevel(1, countItem);
        else if (treeItem->level() == 2)
            countItem(treeItem);
    }

    return counters;
}

bool DiagnosticFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
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
        const bool accepted = filterAcceptsItem(diagnosticItem);
        diagnosticItem->setTextMarkVisible(accepted);
        return accepted;
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
            const int role = Debugger::DetailedErrorView::LocationRole;

            const auto leftLoc = sourceModel()->data(l, role).value<Link>();
            const auto leftText
                = sourceModel()->data(l, ClangToolsDiagnosticModel::TextRole).toString();

            const auto rightLoc = sourceModel()->data(r, role).value<Link>();
            const auto rightText
                = sourceModel()->data(r, ClangToolsDiagnosticModel::TextRole).toString();

            result = std::tie(leftLoc.target.line, leftLoc.target.column, leftText)
                     < std::tie(rightLoc.target.line, rightLoc.target.column, rightText);
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
            = ClangToolsProjectSettings::getSettings(m_project)->suppressedDiagnostics();
    invalidate();
}

bool DiagnosticFilterModel::filterAcceptsItem(const DiagnosticItem *item) const
{
    const Diagnostic &diag = item->diagnostic();

    // Filtered out?
    if (m_filterOptions && !m_filterOptions->checks.contains(diag.name))
        return false;

    // Explicitly suppressed?
    for (const SuppressedDiagnostic &d : std::as_const(m_suppressedDiagnostics)) {
        if (d.description != diag.description)
            continue;
        Utils::FilePath filePath = d.filePath;
        if (d.filePath.isRelativePath())
            filePath = m_lastProjectDirectory.resolvePath(filePath);
        if (filePath == diag.location.targetFilePath)
            return false;
    }
    return true;
}

OptionalFilterOptions DiagnosticFilterModel::filterOptions() const
{
    return m_filterOptions;
}

void DiagnosticFilterModel::setFilterOptions(const OptionalFilterOptions &filterOptions)
{
    m_filterOptions = filterOptions;
    invalidateFilter();
}

} // namespace Internal
} // namespace ClangTools
