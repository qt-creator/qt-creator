// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggersourcepathmappingwidget.h"

#include "commonoptionspage.h"
#include "debuggerengine.h"
#include "debuggertr.h"

#include <utils/buildablehelperlibrary.h>
#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>

using namespace Utils;

namespace Debugger::Internal {

class SourcePathMappingModel;

enum { SourceColumn, TargetColumn, ColumnCount };

using Mapping = QPair<FilePath, FilePath>;

class DebuggerSourcePathMappingWidget : public QGroupBox
{
public:
    DebuggerSourcePathMappingWidget();

    SourcePathMap sourcePathMap() const;
    void setSourcePathMap(const SourcePathMap &);

private:
    void slotAdd();
    void slotAddQt();
    void slotRemove();
    void slotCurrentRowChanged(const QModelIndex &,const QModelIndex &);
    void slotEditSourceFieldChanged();
    void slotEditTargetFieldChanged();

    void resizeColumns();
    void updateEnabled();
    QString editSourceField() const;
    QString editTargetField() const;
    void setEditFieldMapping(const Mapping &m);
    int currentRow() const;
    void setCurrentRow(int r);

    SourcePathMappingModel *m_model;
    QTreeView *m_treeView;
    QPushButton *m_addButton;
    QPushButton *m_addQtButton;
    QPushButton *m_removeButton;
    QLineEdit *m_sourceLineEdit;
    Utils::PathChooser *m_targetChooser;
};

// Qt's various build paths for unpatched versions.
QStringList qtBuildPaths()
{
    if (HostOsInfo::isWindowsHost()) {
        return {"Q:/qt5_workdir/w/s",
                "C:/work/build/qt5_workdir/w/s",
                "c:/users/qt/work/qt",
                "c:/Users/qt/work/install",
                "/Users/qt/work/qt"};
    } else if (HostOsInfo::isMacHost()) {
        return { "/Users/qt/work/qt" };
    } else {
        return { "/home/qt/work/qt" };
    }
}

/*!
    \class Debugger::Internal::SourcePathMappingModel

    \brief The SourcePathMappingModel class is a model for the
    DebuggerSourcePathMappingWidget class.

    Maintains mappings and a dummy placeholder row for adding new mappings.
*/

class SourcePathMappingModel : public QStandardItemModel
{
public:

    explicit SourcePathMappingModel(QObject *parent);

    SourcePathMap sourcePathMap() const;
    void setSourcePathMap(const SourcePathMap &map);

    Mapping mappingAt(int row) const;
    bool isNewPlaceHolderAt(int row) { return isNewPlaceHolder(rawMappingAt(row)); }

    void addMapping(const QString &source, const QString &target)
        { addRawMapping(source, QDir::toNativeSeparators(target)); }

    void addNewMappingPlaceHolder()
        { addRawMapping(m_newSourcePlaceHolder, m_newTargetPlaceHolder); }

    void setSource(int row, const QString &);
    void setTarget(int row, const QString &);

private:
    inline bool isNewPlaceHolder(const Mapping &m) const;
    inline Mapping rawMappingAt(int row) const;
    void addRawMapping(const QString &source, const QString &target);

    const QString m_newSourcePlaceHolder;
    const QString m_newTargetPlaceHolder;
};

SourcePathMappingModel::SourcePathMappingModel(QObject *parent) :
    QStandardItemModel(0, ColumnCount, parent),
    m_newSourcePlaceHolder(Tr::tr("<new source>")),
    m_newTargetPlaceHolder(Tr::tr("<new target>"))
{
    QStringList headers;
    headers.append(Tr::tr("Source path"));
    headers.append(Tr::tr("Target path"));
    setHorizontalHeaderLabels(headers);
}

SourcePathMap SourcePathMappingModel::sourcePathMap() const
{
    SourcePathMap rc;
    const int rows = rowCount();
    for (int r = 0; r < rows; ++r) {
        const Mapping m = mappingAt(r); // Skip placeholders.
        if (!m.first.isEmpty() && !m.second.isEmpty())
            rc.insert(m.first.toString(), m.second.toString());
    }
    return rc;
}

// Check a mapping whether it still contains a placeholder.
bool SourcePathMappingModel::isNewPlaceHolder(const Mapping &m) const
{
    const QChar lessThan('<');
    const QChar greaterThan('>');
    return m.first.isEmpty() || m.first.startsWith(lessThan)
           || m.first.endsWith(greaterThan)
           || m.first.toString() == m_newSourcePlaceHolder
           || m.second.isEmpty() || m.second.startsWith(lessThan)
           || m.second.endsWith(greaterThan)
           || m.second.toString() == m_newTargetPlaceHolder;
}

// Return raw, unfixed mapping
Mapping SourcePathMappingModel::rawMappingAt(int row) const
{
    return Mapping(FilePath::fromUserInput(item(row, SourceColumn)->text()),
                   FilePath::fromUserInput(item(row, TargetColumn)->text()));
}

// Return mapping, empty if it is the place holder.
Mapping SourcePathMappingModel::mappingAt(int row) const
{
    const Mapping raw = rawMappingAt(row);
    return isNewPlaceHolder(raw) ? Mapping() : Mapping(raw.first, raw.second);
}

void SourcePathMappingModel::setSourcePathMap(const SourcePathMap &m)
{
    removeRows(0, rowCount());
    const SourcePathMap::const_iterator cend = m.constEnd();
    for (SourcePathMap::const_iterator it = m.constBegin(); it != cend; ++it)
        addMapping(it.key(), it.value());
}

void SourcePathMappingModel::addRawMapping(const QString &source, const QString &target)
{
    QList<QStandardItem *> items;
    auto sourceItem = new QStandardItem(source);
    sourceItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    auto targetItem = new QStandardItem(target);
    targetItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    items << sourceItem << targetItem;
    appendRow(items);
}

void SourcePathMappingModel::setSource(int row, const QString &s)
{
    QStandardItem *sourceItem = item(row, SourceColumn);
    QTC_ASSERT(sourceItem, return);
    sourceItem->setText(s.isEmpty() ? m_newSourcePlaceHolder : s);
}

void SourcePathMappingModel::setTarget(int row, const QString &t)
{
    QStandardItem *targetItem = item(row, TargetColumn);
    QTC_ASSERT(targetItem, return);
    targetItem->setText(t.isEmpty() ? m_newTargetPlaceHolder : QDir::toNativeSeparators(t));
}

/*!
    \class Debugger::Internal::DebuggerSourcePathMappingWidget

    \brief The DebuggerSourcePathMappingWidget class is a widget for maintaining
    a set of source path mappings for the debugger.

    Path mappings to be applied using source path substitution in GDB.
*/

DebuggerSourcePathMappingWidget::DebuggerSourcePathMappingWidget() :
    m_model(new SourcePathMappingModel(this)),
    m_treeView(new QTreeView(this)),
    m_addButton(new QPushButton(Tr::tr("Add"), this)),
    m_addQtButton(new QPushButton(Tr::tr("Add Qt sources..."), this)),
    m_removeButton(new QPushButton(Tr::tr("Remove"), this)),
    m_sourceLineEdit(new QLineEdit(this)),
    m_targetChooser(new PathChooser(this))
{
    setTitle(Tr::tr("Source Paths Mapping"));
    setToolTip(Tr::tr("<p>Mappings of source file folders to "
                  "be used in the debugger can be entered here.</p>"
                  "<p>This is useful when using a copy of the source tree "
                  "at a location different from the one "
                  "at which the modules where built, for example, while "
                  "doing remote debugging.</p>"
                  "<p>If source is specified as a regular expression by starting it with an "
                  "open parenthesis, the paths in the ELF are matched with the "
                  "regular expression to automatically determine the source path.</p>"
                  "<p>Example: <b>(/home/.*/Project)/KnownSubDir -> D:\\Project</b> will "
                  "substitute ELF built by any user to your local project directory.</p>"));
    // Top list/left part.
    m_treeView->setRootIsDecorated(false);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setModel(m_model);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &DebuggerSourcePathMappingWidget::slotCurrentRowChanged);

    // Top list/Right part: Buttons.
    auto buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_addQtButton);
    m_addQtButton->setVisible(!qtBuildPaths().isEmpty());
    m_addQtButton->setToolTip(Tr::tr("<p>Add a mapping for Qt's source folders "
        "when using an unpatched version of Qt."));
    buttonLayout->addWidget(m_removeButton);
    connect(m_addButton, &QAbstractButton::clicked,
            this, &DebuggerSourcePathMappingWidget::slotAdd);
    connect(m_addQtButton, &QAbstractButton::clicked,
            this, &DebuggerSourcePathMappingWidget::slotAddQt);
    connect(m_removeButton, &QAbstractButton::clicked,
            this, &DebuggerSourcePathMappingWidget::slotRemove);
    buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    // Assemble top
    auto treeHLayout = new QHBoxLayout;
    treeHLayout->addWidget(m_treeView);
    treeHLayout->addLayout(buttonLayout);

    // Edit part
    m_targetChooser->setExpectedKind(PathChooser::ExistingDirectory);
    m_targetChooser->setHistoryCompleter("Debugger.MappingTarget.History");
    connect(m_sourceLineEdit, &QLineEdit::textChanged,
            this, &DebuggerSourcePathMappingWidget::slotEditSourceFieldChanged);
    connect(m_targetChooser, &PathChooser::textChanged,
            this, &DebuggerSourcePathMappingWidget::slotEditTargetFieldChanged);
    auto editLayout = new QFormLayout;
    const QString sourceToolTip = Tr::tr("<p>The source path contained in the "
        "debug information of the executable as reported by the debugger");
    auto editSourceLabel = new QLabel(Tr::tr("&Source path:"));
    editSourceLabel->setToolTip(sourceToolTip);
    m_sourceLineEdit->setToolTip(sourceToolTip);
    editSourceLabel->setBuddy(m_sourceLineEdit);
    editLayout->addRow(editSourceLabel, m_sourceLineEdit);

    const QString targetToolTip = Tr::tr("<p>The actual location of the source "
        "tree on the local machine");
    auto editTargetLabel = new QLabel(Tr::tr("&Target path:"));
    editTargetLabel->setToolTip(targetToolTip);
    editTargetLabel->setBuddy(m_targetChooser);
    m_targetChooser->setToolTip(targetToolTip);
    editLayout->addRow(editTargetLabel, m_targetChooser);
    editLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_targetChooser->lineEdit());

    // Main layout
    auto mainLayout = new QVBoxLayout;
    mainLayout->addLayout(treeHLayout);
    mainLayout->addLayout(editLayout);
    setLayout(mainLayout);
    updateEnabled();
}

QString DebuggerSourcePathMappingWidget::editSourceField() const
{
    return QDir::cleanPath(m_sourceLineEdit->text().trimmed());
}

QString DebuggerSourcePathMappingWidget::editTargetField() const
{
    return m_targetChooser->rawFilePath().toString();
}

void DebuggerSourcePathMappingWidget::setEditFieldMapping(const Mapping &m)
{
    m_sourceLineEdit->setText(m.first.toUserOutput());
    m_targetChooser->setFilePath(m.second);
}

void DebuggerSourcePathMappingWidget::slotCurrentRowChanged
    (const QModelIndex &current, const QModelIndex &)
{
    setEditFieldMapping(current.isValid() ? m_model->mappingAt(current.row()) : Mapping());
    updateEnabled();
}

void DebuggerSourcePathMappingWidget::resizeColumns()
{
    m_treeView->resizeColumnToContents(SourceColumn);
}

void DebuggerSourcePathMappingWidget::updateEnabled()
{
    // Allow for removing the current item.
    const int row = currentRow();
    const bool hasCurrent = row >= 0;
    m_sourceLineEdit->setEnabled(hasCurrent);
    m_targetChooser->setEnabled(hasCurrent);
    m_removeButton->setEnabled(hasCurrent);
    // Allow for adding only if the current item no longer is the place
    // holder for new items.
    const bool canAdd = !hasCurrent || !m_model->isNewPlaceHolderAt(row);
    m_addButton->setEnabled(canAdd);
    m_addQtButton->setEnabled(canAdd);
}

SourcePathMap DebuggerSourcePathMappingWidget::sourcePathMap() const
{
    return m_model->sourcePathMap();
}

void DebuggerSourcePathMappingWidget::setSourcePathMap(const SourcePathMap &m)
{
    m_model->setSourcePathMap(m);
    if (!m.isEmpty())
        resizeColumns();
}

int DebuggerSourcePathMappingWidget::currentRow() const
{
    const QModelIndex index = m_treeView->selectionModel()->currentIndex();
    return index.isValid() ? index.row() : -1;
}

void DebuggerSourcePathMappingWidget::setCurrentRow(int r)
{
    m_treeView->selectionModel()->setCurrentIndex(m_model->index(r, 0),
                                                  QItemSelectionModel::ClearAndSelect
                                                  |QItemSelectionModel::Current
                                                  |QItemSelectionModel::Rows);
}

void DebuggerSourcePathMappingWidget::slotAdd()
{
    m_model->addNewMappingPlaceHolder();
    setCurrentRow(m_model->rowCount() - 1);
}

void DebuggerSourcePathMappingWidget::slotAddQt()
{
    // Add a mapping for various Qt build locations in case of unpatched builds.
    const FilePath qtSourcesPath = FileUtils::getExistingDirectory(this, Tr::tr("Qt Sources"));
    if (qtSourcesPath.isEmpty())
        return;
    for (const QString &buildPath : qtBuildPaths())
        m_model->addMapping(buildPath, qtSourcesPath.toString());
    resizeColumns();
    setCurrentRow(m_model->rowCount() - 1);
}

void DebuggerSourcePathMappingWidget::slotRemove()
{
    const int row = currentRow();
    if (row >= 0)
        m_model->removeRow(row);
}

void DebuggerSourcePathMappingWidget::slotEditSourceFieldChanged()
{
    const int row = currentRow();
    if (row >= 0) {
        m_model->setSource(row, editSourceField());
        updateEnabled();
    }
}

void DebuggerSourcePathMappingWidget::slotEditTargetFieldChanged()
{
    const int row = currentRow();
    if (row >= 0) {
        m_model->setTarget(row, editTargetField());
        updateEnabled();
    }
}

/* Merge settings for an installed Qt (unless another setting is already in the map. */
SourcePathMap mergePlatformQtPath(const DebuggerRunParameters &sp, const SourcePathMap &in)
{
    static const QString qglobal = "qtbase/src/corelib/global/qglobal.h";
    const FilePath sourceLocation = sp.qtSourceLocation;
    if (!(sourceLocation / qglobal).exists())
        return in;

    SourcePathMap rc = in;
    for (const QString &buildPath : qtBuildPaths()) {
        if (!rc.contains(buildPath)) // Do not overwrite user settings.
            rc.insert(buildPath, sourceLocation.path());
    }
    return rc;
}

//
// SourcePathMapAspect
//

class SourcePathMapAspectPrivate
{
public:
    QPointer<DebuggerSourcePathMappingWidget> m_widget;
};


SourcePathMapAspect::SourcePathMapAspect(AspectContainer *container)
    : TypedAspect(container), d(new SourcePathMapAspectPrivate)
{
}

SourcePathMapAspect::~SourcePathMapAspect()
{
    delete d;
}

void SourcePathMapAspect::fromMap(const Store &)
{
    QTC_CHECK(false); // This is only used via read/writeSettings
}

void SourcePathMapAspect::toMap(Store &) const
{
    QTC_CHECK(false);
}

bool SourcePathMapAspect::isDirty()
{
    guiToBuffer();
    return m_internal != m_buffer;
}

void SourcePathMapAspect::addToLayout(Layouting::LayoutItem &parent)
{
    QTC_CHECK(!d->m_widget);
    d->m_widget = createSubWidget<DebuggerSourcePathMappingWidget>();
    d->m_widget->setSourcePathMap(value());
    parent.addItem(d->m_widget.data());
}

bool SourcePathMapAspect::guiToBuffer()
{
    const SourcePathMap old = m_buffer;
    if (d->m_widget)
        m_buffer = d->m_widget->sourcePathMap();
    return m_buffer != old;
}

void SourcePathMapAspect::bufferToGui()
{
    if (d->m_widget)
        d->m_widget->setSourcePathMap(m_buffer);
}

const char sourcePathMappingArrayNameC[] = "SourcePathMappings";
const char sourcePathMappingSourceKeyC[] = "Source";
const char sourcePathMappingTargetKeyC[] = "Target";

void SourcePathMapAspect::writeSettings() const
{
    const SourcePathMap sourcePathMap = value();
    QtcSettings *s = qtcSettings();
    s->beginWriteArray(sourcePathMappingArrayNameC);
    if (!sourcePathMap.isEmpty()) {
        const Key sourcePathMappingSourceKey(sourcePathMappingSourceKeyC);
        const Key sourcePathMappingTargetKey(sourcePathMappingTargetKeyC);
        int i = 0;
        for (auto it = sourcePathMap.constBegin(), cend = sourcePathMap.constEnd();
             it != cend;
             ++it, ++i) {
            s->setArrayIndex(i);
            s->setValue(sourcePathMappingSourceKey, it.key());
            s->setValue(sourcePathMappingTargetKey, it.value());
        }
    }
    s->endArray();
}

void SourcePathMapAspect::readSettings()
{
    QtcSettings *s = qtcSettings();
    SourcePathMap sourcePathMap;
    if (const int count = s->beginReadArray(sourcePathMappingArrayNameC)) {
        const Key sourcePathMappingSourceKey(sourcePathMappingSourceKeyC);
        const Key sourcePathMappingTargetKey(sourcePathMappingTargetKeyC);
        for (int i = 0; i < count; ++i) {
             s->setArrayIndex(i);
             const QString key = s->value(sourcePathMappingSourceKey).toString();
             const QString value = s->value(sourcePathMappingTargetKey).toString();
             sourcePathMap.insert(key, value);
        }
    }
    s->endArray();
    setValue(sourcePathMap);
}

} // Debugger::Internal
