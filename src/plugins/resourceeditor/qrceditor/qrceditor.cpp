// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qrceditor.h"

#include "../resourceeditortr.h"
#include "resourcefile_p.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icore.h>

#include <utils/aggregate.h>
#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QPushButton>
#include <QScopedPointer>
#include <QSlider>
#include <QStackedWidget>
#include <QString>
#include <QUndoCommand>

#ifdef WITH_TESTS
#include <QSignalSpy>
#include <QTest>
#endif

#include <algorithm>
#include <utility>

using namespace Utils;

namespace ResourceEditor::Internal {

constexpr int kMinPreviewSize = 24;
constexpr int kMaxPreviewSize = 256;
constexpr int kDefaultPreviewSize = 64;
// Room around the icon for the wrapped file name below it.
constexpr QSize kPreviewCellPadding(StyleHelper::SpacingTokens::PaddingHXxl,
                                    StyleHelper::SpacingTokens::PaddingVXxl);

class QrcEditorSettings final : public AspectContainer
{
public:
    QrcEditorSettings()
    {
        setSettingsGroup("ResourceEditor");

        iconMode.setSettingsKey("IconMode");

        previewSize.setSettingsKey("PreviewSize");
        previewSize.setRange(kMinPreviewSize, kMaxPreviewSize);
        previewSize.setDefaultValue(kDefaultPreviewSize);

        readSettings();
        connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
                this, [this] { writeSettings(); });
    }

    BoolAspect iconMode{this};
    IntegerAspect previewSize{this};
};

static QrcEditorSettings &settings()
{
    static QrcEditorSettings theSettings;
    return theSettings;
}

class ResourceView : public TreeView
{
    Q_OBJECT

public:
    enum NodeProperty {
        AliasProperty,
        PrefixProperty,
        LanguageProperty
    };

    explicit ResourceView(RelativeResourceModel *model, QUndoStack *history, QWidget *parent = nullptr);

    FilePath filePath() const;

    bool isPrefix(const QModelIndex &index) const;

    QString currentAlias() const;
    QString currentPrefix() const;
    QString currentLanguage() const;
    QString currentResourcePath() const;

    void setResourceDragEnabled(bool e);
    bool resourceDragEnabled() const;

    void findSamePlacePostDeletionModelIndex(int &row, QModelIndex &parent) const;
    QList<QModelIndex> selectedEntriesInViewOrder() const;
    FilePaths existingFilesOf(const QList<QModelIndex> &indexes) const;
    EntryBackup *removeEntry(const QModelIndex &index);
    QStringList existingFilesSubtracted(int prefixIndex, const QStringList &fileNames) const;
    void addFiles(int prefixIndex, const QStringList &fileNames, int cursorFile,
                  int &firstFile, int &lastFile);
    void removeFiles(int prefixIndex, int firstFileIndex, int lastFileIndex);
    QStringList fileNamesToAdd();
    QModelIndex addPrefix();
    QList<QModelIndex> nonExistingFiles();

    void setCurrentAlias(const QString &before, const QString &after);
    void setCurrentPrefix(const QString &before, const QString &after);
    void setCurrentLanguage(const QString &before, const QString &after);
    void advanceMergeId();

    QString getCurrentValue(NodeProperty property) const;
    void changeValue(const QModelIndex &nodeIndex, NodeProperty property, const QString &value);

signals:
    void removeItem();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void addUndoCommand(const QModelIndex &nodeIndex, NodeProperty property,
                        const QString &before, const QString &after);

    RelativeResourceModel *m_qrcModel;

    QUndoStack *m_history;
    int m_mergeId;
};

class IconListView : public ListView
{
    Q_OBJECT

public:
    explicit IconListView(RelativeResourceModel *model);

signals:
    void removeItem();

protected:
    void keyPressEvent(QKeyEvent *e) override;
};

class ResourceIconView : public QWidget
{
public:
    explicit ResourceIconView(RelativeResourceModel *model);

    IconListView *view() const { return m_view; }
    QComboBox *prefixCombo() const { return m_prefixCombo; }

private:
    IconListView *m_view;
    QComboBox *m_prefixCombo;
};

enum ViewPage { ListPage, IconPage };

static bool isRemoveKey(const QKeyEvent *e)
{
    return e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace;
}

class ViewCommand : public QUndoCommand
{
protected:
    ViewCommand(ResourceView *view) : m_view(view) {}

    ResourceView *m_view;
};

class ModelIndexViewCommand : public ViewCommand
{
protected:
    ModelIndexViewCommand(ResourceView *view) : ViewCommand(view) {}

    void storeIndex(const QModelIndex &index)
    {
        if (m_view->isPrefix(index)) {
            m_prefixArrayIndex = index.row();
            m_fileArrayIndex = -1;
        } else {
            m_fileArrayIndex = index.row();
            m_prefixArrayIndex = m_view->model()->parent(index).row();
        }
    }

    QModelIndex makeIndex() const
    {
        const QModelIndex prefixModelIndex
                = m_view->model()->index(m_prefixArrayIndex, 0, QModelIndex());
        if (m_fileArrayIndex != -1) {
            // File node
            const QModelIndex fileModelIndex
                    = m_view->model()->index(m_fileArrayIndex, 0, prefixModelIndex);
            return fileModelIndex;
        } else {
            // Prefix node
            return prefixModelIndex;
        }
    }

private:
    int m_prefixArrayIndex = -1;
    int m_fileArrayIndex = -1;
};

class ModifyPropertyCommand : public ModelIndexViewCommand
{
public:
    ModifyPropertyCommand(ResourceView *view, const QModelIndex &nodeIndex,
                          ResourceView::NodeProperty property, const int mergeId,
                          const QString &before, const QString &after = {})
        : ModelIndexViewCommand(view), m_property(property), m_before(before), m_after(after),
          m_mergeId(mergeId)
    {
        storeIndex(nodeIndex);
    }

private:
    int id() const override { return m_mergeId; }

    bool mergeWith(const QUndoCommand * command) override
    {
        if (command->id() != id() || m_property != static_cast<const ModifyPropertyCommand *>(command)->m_property)
            return false;
        // Choose older command (this) and forgot the other
        return true;
    }

    void undo() override
    {
        // Save current text in m_after for redo()
        m_after = m_view->getCurrentValue(m_property);

        // Reset text to m_before
        m_view->changeValue(makeIndex(), m_property, m_before);
    }

    void redo() override
    {
        // Prevent execution from within QUndoStack::push
        if (m_after.isNull())
            return;

        // Bring back text before undo
        Q_ASSERT(m_view);
        m_view->changeValue(makeIndex(), m_property, m_after);
    }

    ResourceView::NodeProperty m_property;
    QString m_before;
    QString m_after;
    int m_mergeId;
};

class RemoveEntryCommand : public ModelIndexViewCommand
{
public:
    RemoveEntryCommand(ResourceView *view, const QModelIndex &index)
        : ModelIndexViewCommand(view)
    {
        storeIndex(index);
    }

    ~RemoveEntryCommand() override { freeEntry(); }

private:
    void redo() override
    {
        freeEntry();
        const QModelIndex index = makeIndex();
        m_isExpanded = m_view->isExpanded(index);
        m_entry = m_view->removeEntry(index);
    }

    void undo() override
    {
        if (m_entry != nullptr) {
            m_entry->restore();
            Q_ASSERT(m_view != nullptr);
            const QModelIndex index = makeIndex();
            m_view->setExpanded(index, m_isExpanded);
            m_view->setCurrentIndex(index);
            freeEntry();
        }
    }

    void freeEntry() { delete m_entry; m_entry = nullptr; }

    EntryBackup *m_entry = nullptr;
    bool m_isExpanded = true;
};

class RemoveMultipleEntryCommand : public QUndoCommand
{
public:
    // list must be in view order
    RemoveMultipleEntryCommand(ResourceView *view, const QList<QModelIndex> &list)
    {
        m_subCommands.reserve(list.size());
        for (const QModelIndex &index : list)
            m_subCommands.push_back(new RemoveEntryCommand(view, index));
    }

    ~RemoveMultipleEntryCommand() override { qDeleteAll(m_subCommands); }

private:
    void redo() override
    {
        auto it = m_subCommands.rbegin();
        auto end = m_subCommands.rend();

        for (; it != end; ++it)
            (*it)->redo();
    }

    void undo() override
    {
        auto it = m_subCommands.begin();
        auto end = m_subCommands.end();

        for (; it != end; ++it)
            (*it)->undo();
    }

    std::vector<QUndoCommand *> m_subCommands;
};

class AddFilesCommand : public ViewCommand
{
public:
    AddFilesCommand(ResourceView *view, int prefixIndex, int cursorFileIndex,
                    const QStringList &fileNames)
        : ViewCommand(view), m_prefixIndex(prefixIndex), m_cursorFileIndex(cursorFileIndex),
          m_fileNames(fileNames)
    {}

private:
    void redo() override
    {
        m_view->addFiles(m_prefixIndex, m_fileNames, m_cursorFileIndex, m_firstFile, m_lastFile);
    }

    void undo() override
    {
        m_view->removeFiles(m_prefixIndex, m_firstFile, m_lastFile);
    }

    const int m_prefixIndex;
    const int m_cursorFileIndex;
    int m_firstFile = 0;
    int m_lastFile = 0;
    const QStringList m_fileNames;
};

class AddEmptyPrefixCommand : public ViewCommand
{
public:
    AddEmptyPrefixCommand(ResourceView *view) : ViewCommand(view) {}

private:
    void redo() override
    {
        m_prefixArrayIndex = m_view->addPrefix().row();
    }

    void undo() override
    {
        const QModelIndex prefixModelIndex = m_view->model()->index(
                    m_prefixArrayIndex, 0, QModelIndex());
        delete m_view->removeEntry(prefixModelIndex);
    }

    int m_prefixArrayIndex = 0;
};

ResourceView::ResourceView(RelativeResourceModel *model, QUndoStack *history, QWidget *parent) :
    TreeView(parent),
    m_qrcModel(model),
    m_history(history),
    m_mergeId(-1)
{
    advanceMergeId();
    setModel(m_qrcModel);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(ExtendedSelection);
    setEditTriggers(EditKeyPressed);
    setFrameStyle(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    header()->hide();
}

IconListView::IconListView(RelativeResourceModel *model)
{
    setModel(model);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setWordWrap(true);
    setSelectionMode(ExtendedSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameStyle(QFrame::NoFrame);
    setEditTriggers(EditKeyPressed);
}

void IconListView::keyPressEvent(QKeyEvent *e)
{
    if (isRemoveKey(e))
        emit removeItem();
    else
        ListView::keyPressEvent(e);
}

ResourceIconView::ResourceIconView(RelativeResourceModel *model)
    : m_view(new IconListView(model))
    , m_prefixCombo(new QComboBox)
{
    m_prefixCombo->setModel(model);
    m_prefixCombo->setToolTip(Tr::tr("Resource prefix whose files are shown as icons."));

    auto previewSizeSlider = new QSlider(Qt::Horizontal);
    previewSizeSlider->setRange(kMinPreviewSize, kMaxPreviewSize);
    previewSizeSlider->setValue(settings().previewSize());
    previewSizeSlider->setToolTip(Tr::tr("Size of the image previews in the icon view."));

    auto bar = new Utils::StyledBar;
    using namespace Layouting;
    using namespace Utils::StyleHelper::SpacingTokens;
    Row {
        Tr::tr("Prefix:"), m_prefixCombo, st,
        Tr::tr("Preview size:"),
        QtDesignWidgets::IconDisplay { icon(Icons::ZOOMOUT_TOOLBAR) },
        previewSizeSlider,
        QtDesignWidgets::IconDisplay { icon(Icons::ZOOMIN_TOOLBAR) },
        customMargins(PaddingHS, 0, PaddingHS, 0),
        spacing(PrimitiveS),
    }.attachTo(bar);

    Column {
        bar,
        m_view,
        noMargin,
    }.attachTo(this);

    const auto applyPreviewSize = [this](int size) {
        m_view->setIconSize(QSize(size, size));
        m_view->setGridSize(QSize(size, size) + kPreviewCellPadding);
        settings().previewSize.setValue(size);
    };
    connect(previewSizeSlider, &QSlider::valueChanged, this, applyPreviewSize);
    applyPreviewSize(previewSizeSlider->value());

    const auto showPrefix = [this, model](int row) {
        m_view->setRootIndex(model->index(row, 0, {}));
    };
    connect(m_prefixCombo, &QComboBox::currentIndexChanged, this, showPrefix);
    showPrefix(m_prefixCombo->currentIndex());
}

void ResourceView::findSamePlacePostDeletionModelIndex(int &row, QModelIndex &parent) const
{
    // Concept:
    // - Make selection stay on same Y level
    // - Enable user to hit delete several times in row
    const bool hasLowerBrother = m_qrcModel->hasIndex(row + 1, 0, parent);
    if (hasLowerBrother) {
        // First or mid child -> lower brother
        //  o
        //  +--o
        //  +-[o]  <-- deleted
        //  +--o   <-- chosen
        //  o
        // --> return unmodified
    } else {
        if (parent == QModelIndex()) {
            // Last prefix node
            if (row == 0) {
                // Last and only prefix node
                // [o]  <-- deleted
                //  +--o
                //  +--o
                row = -1;
                parent = QModelIndex();
            } else {
                const QModelIndex upperBrother = m_qrcModel->index(row - 1,
                        0, parent);
                if (m_qrcModel->hasChildren(upperBrother)) {
                    //  o
                    //  +--o  <-- selected
                    // [o]    <-- deleted
                    row = m_qrcModel->rowCount(upperBrother) - 1;
                    parent = upperBrother;
                } else {
                    //  o
                    //  o  <-- selected
                    // [o] <-- deleted
                    row--;
                }
            }
        } else {
            // Last file node
            const bool hasPrefixBelow = m_qrcModel->hasIndex(parent.row() + 1,
                    parent.column(), QModelIndex());
            if (hasPrefixBelow) {
                // Last child or parent with lower brother -> lower brother of parent
                //  o
                //  +--o
                //  +-[o]  <-- deleted
                //  o      <-- chosen
                row = parent.row() + 1;
                parent = QModelIndex();
            } else {
                const bool onlyChild = row == 0;
                if (onlyChild) {
                    // Last and only child of last parent -> parent
                    //  o      <-- chosen
                    //  +-[o]  <-- deleted
                    row = parent.row();
                    parent = m_qrcModel->parent(parent);
                } else {
                    // Last child of last parent -> upper brother
                    //  o
                    //  +--o   <-- chosen
                    //  +-[o]  <-- deleted
                    row--;
                }
            }
        }
    }
}

// The selected entries from top to bottom, which is the order
// RemoveMultipleEntryCommand expects. Files below a selected prefix are
// dropped: removing the prefix removes them anyway.
QList<QModelIndex> ResourceView::selectedEntriesInViewOrder() const
{
    const auto viewOrderKey = [this](const QModelIndex &index) {
        if (isPrefix(index))
            return std::make_pair(index.row(), -1);
        return std::make_pair(index.parent().row(), index.row());
    };

    QList<QModelIndex> selected = selectionModel()->selectedRows();
    std::sort(selected.begin(), selected.end(),
              [&](const QModelIndex &a, const QModelIndex &b) {
                  return viewOrderKey(a) < viewOrderKey(b);
              });

    QList<QModelIndex> result;
    for (const QModelIndex &index : std::as_const(selected)) {
        if (!isPrefix(index) && selected.contains(index.parent()))
            continue;
        result.append(index);
    }
    return result;
}

FilePaths ResourceView::existingFilesOf(const QList<QModelIndex> &indexes) const
{
    FilePaths result;
    for (const QModelIndex &index : indexes) {
        if (isPrefix(index))
            continue;
        const FilePath filePath = m_qrcModel->file(index);
        if (filePath.exists())
            result.append(filePath);
    }
    return result;
}

EntryBackup * ResourceView::removeEntry(const QModelIndex &index)
{
    return m_qrcModel->removeEntry(index);
}

QStringList ResourceView::existingFilesSubtracted(int prefixIndex, const QStringList &fileNames) const
{
    return m_qrcModel->existingFilesSubtracted(prefixIndex, fileNames);
}

void ResourceView::addFiles(int prefixIndex, const QStringList &fileNames, int cursorFile,
        int &firstFile, int &lastFile)
{
    m_qrcModel->addFiles(prefixIndex, fileNames, cursorFile, firstFile, lastFile);

    // Expand prefix node
    const QModelIndex prefixModelIndex = m_qrcModel->index(prefixIndex, 0, QModelIndex());
    if (prefixModelIndex.isValid())
        this->setExpanded(prefixModelIndex, true);
}

void ResourceView::removeFiles(int prefixIndex, int firstFileIndex, int lastFileIndex)
{
    Q_ASSERT(prefixIndex >= 0 && prefixIndex < m_qrcModel->rowCount(QModelIndex()));
    const QModelIndex prefixModelIndex = m_qrcModel->index(prefixIndex, 0, QModelIndex());
    Q_ASSERT(prefixModelIndex != QModelIndex());
    Q_ASSERT(firstFileIndex >= 0 && firstFileIndex < m_qrcModel->rowCount(prefixModelIndex));
    Q_ASSERT(lastFileIndex >= 0 && lastFileIndex < m_qrcModel->rowCount(prefixModelIndex));

    for (int i = lastFileIndex; i >= firstFileIndex; i--) {
        const QModelIndex index = m_qrcModel->index(i, 0, prefixModelIndex);
        delete removeEntry(index);
    }
}

void ResourceView::keyPressEvent(QKeyEvent *e)
{
    if (isRemoveKey(e))
        emit removeItem();
    else
        TreeView::keyPressEvent(e);
}

QModelIndex ResourceView::addPrefix()
{
    const QModelIndex idx = m_qrcModel->addNewPrefix();
    selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
    return idx;
}

QList<QModelIndex> ResourceView::nonExistingFiles()
{
    return m_qrcModel->nonExistingFiles();
}

QStringList ResourceView::fileNamesToAdd()
{
    return QFileDialog::getOpenFileNames(this, Tr::tr("Open File"),
            m_qrcModel->filePath().toFSPathString(),
            Tr::tr("All files (*)"));
}

QString ResourceView::currentAlias() const
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return QString();
    return m_qrcModel->alias(current);
}

QString ResourceView::currentPrefix() const
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return QString();
    const QModelIndex preindex = m_qrcModel->prefixIndex(current);
    QString prefix, file;
    m_qrcModel->getItem(preindex, prefix, file);
    return prefix;
}

QString ResourceView::currentLanguage() const
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return QString();
    const QModelIndex preindex = m_qrcModel->prefixIndex(current);
    return m_qrcModel->lang(preindex);
}

QString ResourceView::currentResourcePath() const
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return QString();

    const QString alias = m_qrcModel->alias(current);
    if (!alias.isEmpty())
        return QLatin1Char(':') + currentPrefix() + QLatin1Char('/') + alias;

    return QLatin1Char(':') + currentPrefix() + QLatin1Char('/')
        + m_qrcModel->relativePath(m_qrcModel->file(current));
}

QString ResourceView::getCurrentValue(NodeProperty property) const
{
    switch (property) {
    case AliasProperty: return currentAlias();
    case PrefixProperty: return currentPrefix();
    case LanguageProperty: return currentLanguage();
    default: Q_ASSERT(false); return QString(); // Kill warning
    }
}

void ResourceView::changeValue(const QModelIndex &nodeIndex, NodeProperty property,
        const QString &value)
{
    switch (property) {
    case AliasProperty: m_qrcModel->changeAlias(nodeIndex, value); return;
    case PrefixProperty: m_qrcModel->changePrefix(nodeIndex, value); return;
    case LanguageProperty: m_qrcModel->changeLang(nodeIndex, value); return;
    default: Q_ASSERT(false);
    }
}

void ResourceView::advanceMergeId()
{
    m_mergeId++;
    if (m_mergeId < 0)
        m_mergeId = 0;
}

void ResourceView::addUndoCommand(const QModelIndex &nodeIndex, NodeProperty property,
        const QString &before, const QString &after)
{
    QUndoCommand * const command = new ModifyPropertyCommand(this, nodeIndex, property,
            m_mergeId, before, after);
    m_history->push(command);
}

void ResourceView::setCurrentAlias(const QString &before, const QString &after)
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return;

    addUndoCommand(current, AliasProperty, before, after);
}

void ResourceView::setCurrentPrefix(const QString &before, const QString &after)
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return;
    const QModelIndex preindex = m_qrcModel->prefixIndex(current);

    addUndoCommand(preindex, PrefixProperty, before, after);
}

void ResourceView::setCurrentLanguage(const QString &before, const QString &after)
{
    const QModelIndex current = currentIndex();
    if (!current.isValid())
        return;
    const QModelIndex preindex = m_qrcModel->prefixIndex(current);

    addUndoCommand(preindex, LanguageProperty, before, after);
}

bool ResourceView::isPrefix(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;
    const QModelIndex preindex = m_qrcModel->prefixIndex(index);
    if (preindex == index)
        return true;
    return false;
}

FilePath ResourceView::filePath() const
{
    return m_qrcModel->filePath();
}

void ResourceView::setResourceDragEnabled(bool e)
{
    setDragEnabled(e);
    m_qrcModel->setResourceDragEnabled(e);
}

bool ResourceView::resourceDragEnabled() const
{
    return m_qrcModel->resourceDragEnabled();
}

QrcEditor::QrcEditor(RelativeResourceModel *model, QWidget *parent)
  : Core::MiniSplitter(Qt::Vertical, parent),
    m_model(model),
    m_treeview(new ResourceView(model, &m_history))
{
    m_iconview = new ResourceIconView(model);
    // Share the tree's selection model so the Properties group and the
    // Remove actions operate on the same current item in both views.
    m_iconview->view()->setSelectionModel(m_treeview->selectionModel());

    m_treeview->setFrameStyle(QFrame::NoFrame);
    m_treeview->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    m_viewStack = new QStackedWidget;
    m_viewStack->insertWidget(ListPage, m_treeview);
    m_viewStack->insertWidget(IconPage, m_iconview);

    addWidget(m_viewStack);
    auto widget = new QWidget;
    addWidget(widget);

    auto addPrefixButton = new QPushButton(Tr::tr("Add Prefix"));
    m_addFilesButton = new QPushButton(Tr::tr("Add Files"));
    m_removeButton = new QPushButton(Tr::tr("Remove"));
    m_removeNonExistingButton = new QPushButton(Tr::tr("Remove Missing Files"));

    m_aliasLabel = new QLabel(Tr::tr("Alias:"));
    m_aliasText = new QLineEdit;
    m_prefixLabel = new QLabel(Tr::tr("Prefix:"));
    m_prefixText = new QLineEdit;
    m_languageLabel = new QLabel(Tr::tr("Language:"));
    m_languageText = new QLineEdit;

    using namespace Layouting;
    Column {
        Row {
            addPrefixButton,
            m_addFilesButton,
            m_removeButton,
            m_removeNonExistingButton,
            st,
        },
        Group {
            title(Tr::tr("Properties")),
            Form {
                m_aliasLabel, m_aliasText, br,
                m_prefixLabel, m_prefixText, br,
                m_languageLabel, m_languageText, br,
            }
        },
        st,
    }.attachTo(widget);

    connect(addPrefixButton, &QAbstractButton::clicked, this, &QrcEditor::onAddPrefix);
    connect(m_addFilesButton, &QAbstractButton::clicked, this, &QrcEditor::onAddFiles);
    connect(m_removeButton, &QAbstractButton::clicked, this, &QrcEditor::onRemove);
    connect(m_removeNonExistingButton, &QPushButton::clicked,
            this, &QrcEditor::onRemoveNonExisting);

    connect(m_iconview->prefixCombo(), &QComboBox::currentIndexChanged, this, [this] {
        if (isIconMode())
            limitSelectionToShownPrefix();
    });
    connect(m_treeview->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current) {
        if (current.isValid()) {
            m_iconview->prefixCombo()->setCurrentIndex(m_model->prefixIndex(current).row());
        } else if (isIconMode()) {
            // The last file of the shown prefix is gone, stay on the prefix.
            m_treeview->selectionModel()->setCurrentIndex(m_iconview->view()->rootIndex(),
                                                          QItemSelectionModel::NoUpdate);
        }
    });
    connect(m_treeview->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
        // The icon view never shows its prefix row, so it must not stay selected.
        QItemSelectionModel *selection = m_treeview->selectionModel();
        const QModelIndex root = currentView()->rootIndex();
        if (selection->isSelected(root))
            selection->select(root, QItemSelectionModel::Deselect);
    });
    setIconMode(settings().iconMode());

    connect(m_treeview, &ResourceView::removeItem, this, &QrcEditor::onRemove);
    connect(m_treeview->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &QrcEditor::updateCurrent);
    connect(m_treeview->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &QrcEditor::updateCurrent);
    connect(m_iconview->view(), &IconListView::removeItem, this, &QrcEditor::onRemove);
    connectFileSignals(m_treeview);
    connectFileSignals(m_iconview->view());
    m_treeview->setFocus();

    connect(m_aliasText, &QLineEdit::textEdited, this, &QrcEditor::onAliasChanged);
    connect(m_prefixText, &QLineEdit::textEdited, this, &QrcEditor::onPrefixChanged);
    connect(m_languageText, &QLineEdit::textEdited, this, &QrcEditor::onLanguageChanged);

    // Prevent undo command merging after a switch of focus:
    // (0) The initial text is "Green".
    // (1) The user appends " is a color." --> text is "Green is a color."
    // (2) The user clicks into some other line edit --> loss of focus
    // (3) The user gives focuse again and substitutes "Green" with "Red"
    //     --> text now is "Red is a color."
    // (4) The user hits undo --> text now is "Green is a color."
    //     Without calling advanceMergeId() it would have been "Green", instead.
    connect(m_aliasText, &QLineEdit::editingFinished,
            m_treeview, &ResourceView::advanceMergeId);
    connect(m_prefixText, &QLineEdit::editingFinished,
            m_treeview, &ResourceView::advanceMergeId);
    connect(m_languageText, &QLineEdit::editingFinished,
            m_treeview, &ResourceView::advanceMergeId);

    connect(&m_history, &QUndoStack::canRedoChanged, this, &QrcEditor::updateHistoryControls);
    connect(&m_history, &QUndoStack::canUndoChanged, this, &QrcEditor::updateHistoryControls);

    auto agg = new Aggregation::Aggregate;
    agg->add(m_treeview);
    agg->add(new Core::ItemViewFind(m_treeview));
    auto iconAgg = new Aggregation::Aggregate;
    iconAgg->add(m_iconview->view());
    iconAgg->add(new Core::ItemViewFind(m_iconview->view()));

    updateHistoryControls();
    updateCurrent();
}

QrcEditor::~QrcEditor() = default;

void QrcEditor::connectFileSignals(QAbstractItemView *view)
{
    connect(view, &QAbstractItemView::activated, this, [this](const QModelIndex &index) {
        const FilePath filePath = m_model->file(index);
        if (!filePath.isEmpty())
            emit itemActivated(filePath);
    });
    connect(view, &QWidget::customContextMenuRequested, this, [this, view](const QPoint &pos) {
        const FilePath filePath = m_model->file(view->indexAt(pos));
        if (!filePath.isEmpty())
            emit showContextMenu(view->viewport()->mapToGlobal(pos), filePath);
    });
}

QAbstractItemView *QrcEditor::currentView() const
{
    if (isIconMode())
        return m_iconview->view();
    return m_treeview;
}

// The selection is shared with the tree, so it has to leave the prefixes
// the icon view does not show, or Remove and the Properties fields would
// act on items the user cannot see.
void QrcEditor::limitSelectionToShownPrefix()
{
    const QModelIndex root = m_iconview->view()->rootIndex();
    QItemSelectionModel *selection = m_treeview->selectionModel();
    QItemSelection hidden;
    for (const QModelIndex &index : selection->selectedIndexes()) {
        if (index == root || m_model->prefixIndex(index) != root)
            hidden.select(index, index);
    }
    selection->select(hidden, QItemSelectionModel::Deselect);
    if (m_model->prefixIndex(selection->currentIndex()) != root)
        selection->setCurrentIndex(root, QItemSelectionModel::NoUpdate);
}

bool QrcEditor::isIconMode() const
{
    return m_viewStack->currentIndex() == IconPage;
}

void QrcEditor::setIconMode(bool on)
{
    settings().iconMode.setValue(on);
    m_viewStack->setCurrentIndex(on ? IconPage : ListPage);
    if (on) {
        limitSelectionToShownPrefix();
        m_iconview->view()->setFocus();
    }
    updateCurrent();
}

void QrcEditor::loaded(bool success)
{
    if (!success)
        return;
    // Set "focus"
    m_treeview->setCurrentIndex(m_treeview->model()->index(0,0));
    // Expand prefix nodes
    m_treeview->expandAll();
}

void QrcEditor::refresh()
{
    m_model->refresh();
}

// Propagates a change of selection in the tree
// to the alias/prefix/language edit controls
void QrcEditor::updateCurrent()
{
    // The property fields edit the current entry, so they are only offered
    // while that entry is also the only selected one.
    const QModelIndexList selected = m_treeview->selectionModel()->selectedRows();
    const QModelIndex current = m_treeview->currentIndex();
    // The icon view keeps its prefix out of the selection, but it is still
    // the entry being edited while nothing else is selected.
    const bool iconPrefix = selected.isEmpty() && current.isValid()
                            && current == currentView()->rootIndex();
    const bool single = iconPrefix || (selected.size() == 1 && selected.first() == current);
    const bool isPrefix = single && m_treeview->isPrefix(current);
    const bool isFile = !isPrefix && single;

    m_aliasLabel->setEnabled(isFile);
    m_aliasText->setEnabled(isFile);
    m_currentAlias = m_treeview->currentAlias();
    m_aliasText->setText(m_currentAlias);

    m_prefixLabel->setEnabled(isPrefix);
    m_prefixText->setEnabled(isPrefix);
    m_currentPrefix = m_treeview->currentPrefix();
    m_prefixText->setText(m_currentPrefix);

    m_languageLabel->setEnabled(isPrefix);
    m_languageText->setEnabled(isPrefix);
    m_currentLanguage = m_treeview->currentLanguage();
    m_languageText->setText(m_currentLanguage);

    m_addFilesButton->setEnabled(current.isValid());
    m_removeButton->setEnabled(!selected.isEmpty());
}

void QrcEditor::updateHistoryControls()
{
    emit undoStackChanged(m_history.canUndo(), m_history.canRedo());
}

// Helper for resolveLocationIssues():
// For code clarity, a context with convenience functions to execute
// the dialogs required for checking the image file paths
// (and keep them around for file dialog execution speed).
// Basically, resolveLocationIssues() checks the paths of the images
// and asks the user to copy them into the resource file location.
// When the user does a multiselection of files, this requires popping
// up the dialog several times in a row.
struct ResolveLocationContext {
    QAbstractButton *execLocationMessageBox(QWidget *parent,
                                        const QString &file,
                                        bool wantSkipButton);
    QString execCopyFileDialog(QWidget *parent,
                               const QDir &dir,
                               const QString &targetPath);

    QScopedPointer<QMessageBox> messageBox;
    QScopedPointer<QFileDialog> copyFileDialog;

    QPushButton *copyButton = nullptr;
    QPushButton *skipButton = nullptr;
    QPushButton *abortButton = nullptr;
};

QAbstractButton *ResolveLocationContext::execLocationMessageBox(QWidget *parent,
                                                            const QString &file,
                                                            bool wantSkipButton)
{
    if (messageBox.isNull()) {
        messageBox.reset(new QMessageBox(QMessageBox::Warning,
                                         Tr::tr("Invalid file location"),
                                         QString(), QMessageBox::NoButton, parent));
        copyButton = messageBox->addButton(Tr::tr("Copy"), QMessageBox::ActionRole);
        abortButton = messageBox->addButton(Tr::tr("Abort"), QMessageBox::RejectRole);
        messageBox->setDefaultButton(copyButton);
    }
    if (wantSkipButton && !skipButton) {
        skipButton = messageBox->addButton(Tr::tr("Skip"), QMessageBox::DestructiveRole);
        messageBox->setEscapeButton(skipButton);
    }
    messageBox->setText(Tr::tr("The file %1 is not in a subdirectory of the resource file. "
                               "You now have the option to copy this file to a valid location.")
                    .arg(QDir::toNativeSeparators(file)));
    messageBox->exec();
    return messageBox->clickedButton();
}

QString ResolveLocationContext::execCopyFileDialog(QWidget *parent, const QDir &dir, const QString &targetPath)
{
    // Delayed creation of file dialog.
    if (copyFileDialog.isNull()) {
        copyFileDialog.reset(new QFileDialog(parent, Tr::tr("Choose Copy Location")));
        copyFileDialog->setFileMode(QFileDialog::AnyFile);
        copyFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    }
    copyFileDialog->selectFile(targetPath);
    // Repeat until the path entered is no longer above 'dir'
    // (relative is not "../").
    while (true) {
        if (copyFileDialog->exec() != QDialog::Accepted)
            return QString();
        const QStringList files = copyFileDialog->selectedFiles();
        if (files.isEmpty())
            return QString();
        const QString relPath = dir.relativeFilePath(files.front());
        if (!relPath.startsWith(QLatin1String("../")))
            return files.front();
    }
    return QString();
}

// Helper to copy a file with message boxes
static inline bool copyFile(const QString &file, const QString &copyName, QWidget *parent)
{
    if (QFileInfo::exists(copyName)) {
        if (!QFile::remove(copyName)) {
            QMessageBox::critical(parent, Tr::tr("Overwriting Failed"),
                                  Tr::tr("Could not overwrite file %1.")
                                  .arg(QDir::toNativeSeparators(copyName)));
            return false;
        }
    }
    if (!QFile::copy(file, copyName)) {
        QMessageBox::critical(parent, Tr::tr("Copying Failed"),
                              Tr::tr("Could not copy the file to %1.")
                              .arg(QDir::toNativeSeparators(copyName)));
        return false;
    }
    return true;
}

void QrcEditor::resolveLocationIssues(QStringList &files)
{
    const QDir dir = m_treeview->filePath().toFileInfo().absoluteDir();
    const QString dotdotSlash = QLatin1String("../");
    int i = 0;
    const int count = files.count();
    const int initialCount = files.count();

    // Find first troublesome file
    for (; i < count; i++) {
        QString const &file = files.at(i);
        const QString relativePath = dir.relativeFilePath(file);
        if (relativePath.startsWith(dotdotSlash))
            break;
    }

    // All paths fine -> no interaction needed
    if (i == count)
        return;

    // Interact with user from now on
    ResolveLocationContext context;
    bool abort = false;
    for (QStringList::iterator it = files.begin(); it != files.end(); ) {
        // Path fine -> skip file
        QString const &file = *it;
        QString const relativePath = dir.relativeFilePath(file);
        if (!relativePath.startsWith(dotdotSlash))
            continue;
        // Path troublesome and aborted -> remove file
        bool ok = false;
        if (!abort) {
            // Path troublesome -> query user "Do you want copy/abort/skip".
            QAbstractButton *clickedButton = context.execLocationMessageBox(this, file, initialCount > 1);
            if (clickedButton == context.copyButton) {
                const QFileInfo fi(file);
                QFileInfo suggestion;
                QDir tmpTarget(dir.path() + QLatin1Char('/') + QLatin1String("Resources"));
                if (tmpTarget.exists())
                    suggestion.setFile(tmpTarget, fi.fileName());
                else
                    suggestion.setFile(dir, fi.fileName());
                // Prompt for copy location, copy and replace name.
                const QString copyName = context.execCopyFileDialog(this, dir, suggestion.absoluteFilePath());
                ok = !copyName.isEmpty() && copyFile(file, copyName, this);
                if (ok)
                    *it = copyName;
            } else if (clickedButton == context.abortButton) {
                abort = true;
            }
        } // !abort
        if (ok) { // Remove files where user canceled or failures occurred.
            ++it;
        } else {
            it = files.erase(it);
        }
    } // for files
}

void QrcEditor::setResourceDragEnabled(bool e)
{
    m_treeview->setResourceDragEnabled(e);
    m_iconview->view()->setDragEnabled(e); // after setMovement(), which resets it
}

bool QrcEditor::resourceDragEnabled() const
{
    return m_treeview->resourceDragEnabled();
}

void QrcEditor::editCurrentItem()
{
    const QModelIndex current = m_treeview->selectionModel()->currentIndex();
    if (current.isValid())
        currentView()->edit(current);
}

QString QrcEditor::currentResourcePath() const
{
    return m_treeview->currentResourcePath();
}

// Slot for change of line edit content 'alias'
void QrcEditor::onAliasChanged(const QString &alias)
{
    const QString &before = m_currentAlias;
    const QString &after = alias;
    m_treeview->setCurrentAlias(before, after);
    m_currentAlias = alias;
    updateHistoryControls();
}

// Slot for change of line edit content 'prefix'
void QrcEditor::onPrefixChanged(const QString &prefix)
{
    const QString &before = m_currentPrefix;
    const QString &after = prefix;
    m_treeview->setCurrentPrefix(before, after);
    m_currentPrefix = prefix;
    updateHistoryControls();
}

// Slot for change of line edit content 'language'
void QrcEditor::onLanguageChanged(const QString &language)
{
    const QString &before = m_currentLanguage;
    const QString &after = language;
    m_treeview->setCurrentLanguage(before, after);
    m_currentLanguage = language;
    updateHistoryControls();
}

static bool confirmRemoval(QWidget *parent, const FilePaths &files, bool *deleteFromDisk)
{
    if (files.size() == 1) {
        RemoveFileDialog dialog(files.first());
        if (dialog.exec() != QDialog::Accepted)
            return false;
        *deleteFromDisk = dialog.isDeleteFileChecked();
        return true;
    }

    QMessageBox box(QMessageBox::Question,
                    Tr::tr("Remove Files"),
                    Tr::tr("Remove these %n files from the resource file?", nullptr, files.size()),
                    QMessageBox::Yes | QMessageBox::No,
                    parent);
    box.setDetailedText(Utils::transform(files, &FilePath::toUserOutput).join('\n'));
    auto deleteFileCheckBox = new QCheckBox(Tr::tr("&Delete files permanently"));
    box.setCheckBox(deleteFileCheckBox);
    if (box.exec() != QMessageBox::Yes)
        return false;
    *deleteFromDisk = deleteFileCheckBox->isChecked();
    return true;
}

// Slot for 'Remove' button
void QrcEditor::onRemove()
{
    const QList<QModelIndex> selected = m_treeview->selectedEntriesInViewOrder();
    if (selected.isEmpty())
        return;

    // Ask once for the whole selection, not once per entry.
    const FilePaths files = m_treeview->existingFilesOf(selected);
    bool deleteFromDisk = false;
    if (!files.isEmpty() && !confirmRemoval(this, files, &deleteFromDisk))
        return;

    if (selected.size() == 1) {
        // A single removal keeps the cursor on the next sensible item.
        const QModelIndex current = selected.first();
        int afterDeletionArrayIndex = current.row();
        QModelIndex afterDeletionParent = current.parent();
        // The icon view only shows one prefix, so stay inside it.
        if (isIconMode() && afterDeletionParent.isValid()) {
            afterDeletionArrayIndex
                    = qMin(current.row(), m_model->rowCount(afterDeletionParent) - 2);
        } else {
            m_treeview->findSamePlacePostDeletionModelIndex(afterDeletionArrayIndex,
                                                            afterDeletionParent);
        }
        m_history.push(new RemoveEntryCommand(m_treeview, current));
        const QModelIndex afterDeletionModelIndex
                = m_treeview->model()->index(afterDeletionArrayIndex, 0, afterDeletionParent);
        m_treeview->setCurrentIndex(afterDeletionModelIndex);
    } else {
        m_history.push(new RemoveMultipleEntryCommand(m_treeview, selected));
    }
    if (!files.isEmpty())
        Core::FileUtils::removeFiles(files, deleteFromDisk);
    updateHistoryControls();
}

// Slot for 'Remove missing files' button
void QrcEditor::onRemoveNonExisting()
{
    QList<QModelIndex> toRemove = m_treeview->nonExistingFiles();

    QUndoCommand * const removeCommand = new RemoveMultipleEntryCommand(m_treeview, toRemove);
    m_history.push(removeCommand);
    updateHistoryControls();
}

// Slot for 'Add File' button
void QrcEditor::onAddFiles()
{
    QModelIndex const current = m_treeview->currentIndex();
    int const currentIsPrefixNode = m_treeview->isPrefix(current);
    int const prefixArrayIndex = currentIsPrefixNode ? current.row()
            : m_treeview->model()->parent(current).row();
    int const cursorFileArrayIndex = currentIsPrefixNode ? 0 : current.row();
    const QStringList fileNames = m_treeview->fileNamesToAdd();
    QStringList uniqueFileNames = m_treeview->existingFilesSubtracted(prefixArrayIndex, fileNames);

    // Files already present in the prefix are dropped silently otherwise.
    QStringList alreadyPresent;
    for (const QString &file : fileNames) {
        if (!uniqueFileNames.contains(file))
            alreadyPresent.append(QFileInfo(file).fileName());
    }
    if (!alreadyPresent.isEmpty()) {
        QMessageBox::information(
            this,
            Tr::tr("Files Already Present"),
            Tr::tr("The following files are already in the resource and were not added again:")
                + "\n\n" + alreadyPresent.join('\n'));
    }

    resolveLocationIssues(uniqueFileNames);
    if (uniqueFileNames.isEmpty())
        return;
    QUndoCommand * const addFilesCommand = new AddFilesCommand(
            m_treeview, prefixArrayIndex, cursorFileArrayIndex, uniqueFileNames);
    m_history.push(addFilesCommand);
    updateHistoryControls();
}

// Slot for 'Add Prefix' button
void QrcEditor::onAddPrefix()
{
    QUndoCommand * const addEmptyPrefixCommand = new AddEmptyPrefixCommand(m_treeview);
    m_history.push(addEmptyPrefixCommand);
    updateHistoryControls();
    m_prefixText->selectAll();
    m_prefixText->setFocus();
}

// Slot for 'Undo' button
void QrcEditor::onUndo()
{
    m_history.undo();
    updateCurrent();
    updateHistoryControls();
}

// Slot for 'Redo' button
void QrcEditor::onRedo()
{
    m_history.redo();
    updateCurrent();
    updateHistoryControls();
}

#ifdef WITH_TESTS

class QrcEditorTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testIconViewFollowsTreeCurrent();
    void testSwitchingPrefixKeepsItsSelection();
    void testIconModeDropsHiddenSelection();
    void testIconModeDropsSelectedPrefix();
    void testRemoveInIconModeStaysInPrefix();
    void testRenameEditsInIconView();
    void testDoubleClickActivatesInIconView();
    void testIconViewKeepsSelectionAfterRefresh();
    void testNoContextMenuOnEmptySpace();
    void testDeleteKeyRemovesInIconView();
    void testRecheckNotifiesViews();
    void testPrefixFieldsFollowModeSwitch();
    void testAddPrefixInIconModeIsEditable();
    void testRemoveAllFilesInIconModeStaysInPrefix();
    void testIconViewDragFollowsSetting();
    void testPrefixChangeUpdatesCombo();
    void testUndoAddPrefixInIconModeRebuildsCombo();

private:
    QModelIndex prefix(int row) const { return m_model->index(row, 0, {}); }
    QModelIndex file(int prefixRow, int row) const
    {
        return m_model->index(row, 0, prefix(prefixRow));
    }
    QItemSelectionModel *selection() const { return m_editor->m_treeview->selectionModel(); }
    QComboBox *combo() const { return m_editor->m_iconview->prefixCombo(); }
    IconListView *iconView() const { return m_editor->m_iconview->view(); }

    bool m_savedIconMode = false;
    RelativeResourceModel *m_model = nullptr;
    QrcEditor *m_editor = nullptr;
};

void QrcEditorTest::initTestCase()
{
    m_savedIconMode = settings().iconMode();
}

void QrcEditorTest::cleanupTestCase()
{
    settings().iconMode.setValue(m_savedIconMode);
}

void QrcEditorTest::init()
{
    m_model = new RelativeResourceModel;
    m_model->setFilePath(FilePath::fromString("/qrcedit/test.qrc"));
    m_model->insertPrefix(0, "/a", {});
    m_model->insertFile(0, 0, "/qrcedit/a1.png", {});
    m_model->insertFile(0, 1, "/qrcedit/a2.png", {});
    m_model->insertPrefix(1, "/b", {});
    m_model->insertFile(1, 0, "/qrcedit/b1.png", {});
    settings().iconMode.setValue(false);
    m_editor = new QrcEditor(m_model);
}

void QrcEditorTest::cleanup()
{
    delete m_editor;
    m_editor = nullptr;
    delete m_model;
    m_model = nullptr;
}

void QrcEditorTest::testIconViewFollowsTreeCurrent()
{
    m_editor->setIconMode(true);
    QCOMPARE(combo()->currentIndex(), 0);

    selection()->setCurrentIndex(file(1, 0), QItemSelectionModel::ClearAndSelect);

    QCOMPARE(combo()->currentIndex(), 1);
    QCOMPARE(iconView()->rootIndex(), prefix(1));
}

void QrcEditorTest::testSwitchingPrefixKeepsItsSelection()
{
    m_editor->setIconMode(true);
    selection()->setCurrentIndex(file(0, 0), QItemSelectionModel::ClearAndSelect);
    selection()->select(file(1, 0), QItemSelectionModel::Select);

    combo()->setCurrentIndex(1);

    QVERIFY(!selection()->isSelected(file(0, 0)));
    QVERIFY(selection()->isSelected(file(1, 0)));
    QCOMPARE(selection()->currentIndex(), prefix(1));
}

void QrcEditorTest::testIconModeDropsHiddenSelection()
{
    selection()->setCurrentIndex(file(0, 0), QItemSelectionModel::ClearAndSelect);
    selection()->select(file(1, 0), QItemSelectionModel::Select);

    m_editor->setIconMode(true);

    QCOMPARE(combo()->currentIndex(), 0);
    QVERIFY(selection()->isSelected(file(0, 0)));
    QVERIFY(!selection()->isSelected(file(1, 0)));
}

void QrcEditorTest::testIconModeDropsSelectedPrefix()
{
    selection()->setCurrentIndex(prefix(0), QItemSelectionModel::ClearAndSelect);

    m_editor->setIconMode(true);
    QVERIFY(!selection()->isSelected(prefix(0)));

    selection()->setCurrentIndex(prefix(0), QItemSelectionModel::ClearAndSelect);
    QVERIFY(!selection()->isSelected(prefix(0)));
}

void QrcEditorTest::testRemoveInIconModeStaysInPrefix()
{
    m_model->insertFile(0, 2, "/qrcedit/a3.png", {});
    m_editor->setIconMode(true);

    selection()->setCurrentIndex(file(0, 2), QItemSelectionModel::ClearAndSelect);
    m_editor->m_removeButton->click();
    QCOMPARE(iconView()->rootIndex(), prefix(0));
    QCOMPARE(selection()->currentIndex(), file(0, 1));

    selection()->setCurrentIndex(file(0, 1), QItemSelectionModel::ClearAndSelect);
    m_editor->m_removeButton->click();
    selection()->setCurrentIndex(file(0, 0), QItemSelectionModel::ClearAndSelect);
    m_editor->m_removeButton->click();
    QCOMPARE(iconView()->rootIndex(), prefix(0));
    QCOMPARE(combo()->currentIndex(), 0);
    QCOMPARE(selection()->currentIndex(), prefix(0));
    QVERIFY(!selection()->isSelected(prefix(0)));
}

void QrcEditorTest::testRenameEditsInIconView()
{
    m_editor->setIconMode(true);
    selection()->setCurrentIndex(file(0, 1), QItemSelectionModel::ClearAndSelect);

    m_editor->editCurrentItem();

    QVERIFY(iconView()->viewport()->findChild<QLineEdit *>());
    QVERIFY(!m_editor->m_treeview->viewport()->findChild<QLineEdit *>());
}

void QrcEditorTest::testDoubleClickActivatesInIconView()
{
    m_editor->setIconMode(true);
    m_editor->resize(400, 400);
    m_editor->show();
    QAbstractItemView *view = iconView();
    QTRY_VERIFY(view->visualRect(file(0, 0)).isValid());
    const QPoint center = view->visualRect(file(0, 0)).center();
    QCOMPARE(view->indexAt(center), file(0, 0));
    QSignalSpy spy(m_editor, &QrcEditor::itemActivated);

    QTest::mouseClick(view->viewport(), Qt::LeftButton, {}, center);
    QTest::mouseDClick(view->viewport(), Qt::LeftButton, {}, center);

    QCOMPARE(spy.count(), 1);
    QVERIFY(!view->viewport()->findChild<QLineEdit *>());
}

void QrcEditorTest::testIconViewKeepsSelectionAfterRefresh()
{
    m_editor->setIconMode(true);

    m_editor->refresh();

    QCOMPARE(iconView()->selectionModel(), selection());
    selection()->setCurrentIndex(file(1, 0), QItemSelectionModel::ClearAndSelect);
    QCOMPARE(combo()->currentIndex(), 1);
}

void QrcEditorTest::testNoContextMenuOnEmptySpace()
{
    m_editor->setIconMode(true);
    QSignalSpy spy(m_editor, &QrcEditor::showContextMenu);

    emit iconView()->customContextMenuRequested(QPoint(10000, 10000));

    QCOMPARE(spy.count(), 0);
}

void QrcEditorTest::testDeleteKeyRemovesInIconView()
{
    m_editor->setIconMode(true);
    selection()->setCurrentIndex(file(0, 0), QItemSelectionModel::ClearAndSelect);

    QTest::keyClick(iconView(), Qt::Key_Delete);

    QCOMPARE(m_model->rowCount(prefix(0)), 1);
}

void QrcEditorTest::testRecheckNotifiesViews()
{
    QSignalSpy spy(m_model, &QAbstractItemModel::dataChanged);

    m_editor->refresh();

    QVERIFY(spy.count() > 0);
    QVERIFY(spy.last().at(2).value<QList<int>>().contains(Qt::DecorationRole));
}

void QrcEditorTest::testPrefixFieldsFollowModeSwitch()
{
    m_editor->setIconMode(true);
    combo()->setCurrentIndex(1);
    QVERIFY(m_editor->m_prefixText->isEnabled());

    m_editor->setIconMode(false);

    QVERIFY(!m_editor->m_prefixText->isEnabled());
}

void QrcEditorTest::testAddPrefixInIconModeIsEditable()
{
    m_editor->setIconMode(true);

    m_editor->onAddPrefix();

    QVERIFY(m_editor->m_prefixText->isEnabled());
    QVERIFY(m_editor->m_languageText->isEnabled());
}

void QrcEditorTest::testRemoveAllFilesInIconModeStaysInPrefix()
{
    m_editor->setIconMode(true);
    selection()->setCurrentIndex(file(0, 0), QItemSelectionModel::ClearAndSelect);
    selection()->select(file(0, 1), QItemSelectionModel::Select);

    m_editor->m_removeButton->click();

    QCOMPARE(selection()->currentIndex(), prefix(0));
    QVERIFY(m_editor->m_addFilesButton->isEnabled());
}

void QrcEditorTest::testIconViewDragFollowsSetting()
{
    m_editor->setResourceDragEnabled(true);
    QVERIFY(iconView()->dragEnabled());

    m_editor->setResourceDragEnabled(false);
    QVERIFY(!iconView()->dragEnabled());
}

void QrcEditorTest::testPrefixChangeUpdatesCombo()
{
    m_editor->setIconMode(true);

    m_model->changePrefix(prefix(0), "/z");

    QCOMPARE(combo()->itemText(0), prefix(0).data().toString());
    QVERIFY(combo()->itemText(0).contains("/z"));
}

void QrcEditorTest::testUndoAddPrefixInIconModeRebuildsCombo()
{
    m_editor->setIconMode(true);
    m_editor->onAddPrefix();
    QCOMPARE(combo()->count(), 3);

    m_editor->onUndo();

    QCOMPARE(combo()->count(), 2);
    QVERIFY(combo()->currentIndex() >= 0);
    QCOMPARE(iconView()->rootIndex(), prefix(combo()->currentIndex()));
}

QObject *createQrcEditorTest()
{
    return new QrcEditorTest;
}

#endif // WITH_TESTS

} // ResourceEditor::Internal

#include "qrceditor.moc"
