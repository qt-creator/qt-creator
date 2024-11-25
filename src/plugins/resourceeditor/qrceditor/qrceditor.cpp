// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qrceditor.h"

#include "../resourceeditortr.h"
#include "resourcefile_p.h"

#include <aggregation/aggregate.h>

#include <coreplugin/find/itemviewfind.h>

#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>

#include <QDebug>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QPushButton>
#include <QScopedPointer>
#include <QString>
#include <QUndoCommand>

using namespace Utils;

namespace ResourceEditor::Internal {

class ResourceView : public Utils::TreeView
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
    EntryBackup *removeEntry(const QModelIndex &index);
    QStringList existingFilesSubtracted(int prefixIndex, const QStringList &fileNames) const;
    void addFiles(int prefixIndex, const QStringList &fileNames, int cursorFile,
                  int &firstFile, int &lastFile);
    void removeFiles(int prefixIndex, int firstFileIndex, int lastFileIndex);
    QStringList fileNamesToAdd();
    QModelIndex addPrefix();
    QList<QModelIndex> nonExistingFiles();

    void refresh();

    void setCurrentAlias(const QString &before, const QString &after);
    void setCurrentPrefix(const QString &before, const QString &after);
    void setCurrentLanguage(const QString &before, const QString &after);
    void advanceMergeId();

    QString getCurrentValue(NodeProperty property) const;
    void changeValue(const QModelIndex &nodeIndex, NodeProperty property, const QString &value);

signals:
    void removeItem();
    void itemActivated(const QString &fileName);
    void contextMenuShown(const QPoint &globalPos, const QString &fileName);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void onItemActivated(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);
    void addUndoCommand(const QModelIndex &nodeIndex, NodeProperty property,
                        const QString &before, const QString &after);

    RelativeResourceModel *m_qrcModel;

    QUndoStack *m_history;
    int m_mergeId;
};

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
    int m_prefixArrayIndex;
    int m_fileArrayIndex;
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

    int m_prefixIndex;
    int m_cursorFileIndex;
    int m_firstFile;
    int m_lastFile;
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

    int m_prefixArrayIndex;
};

ResourceView::ResourceView(RelativeResourceModel *model, QUndoStack *history, QWidget *parent) :
    Utils::TreeView(parent),
    m_qrcModel(model),
    m_history(history),
    m_mergeId(-1)
{
    advanceMergeId();
    setModel(m_qrcModel);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(EditKeyPressed);
    setFrameStyle(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    header()->hide();

    connect(this, &QWidget::customContextMenuRequested, this, &ResourceView::showContextMenu);
    connect(this, &QAbstractItemView::activated, this, &ResourceView::onItemActivated);
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
    if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace)
        emit removeItem();
    else
        Utils::TreeView::keyPressEvent(e);
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

void ResourceView::refresh()
{
    m_qrcModel->refresh();
    QModelIndex idx = currentIndex();
    setModel(nullptr);
    setModel(m_qrcModel);
    setCurrentIndex(idx);
    expandAll();
}

QStringList ResourceView::fileNamesToAdd()
{
    return QFileDialog::getOpenFileNames(this, Tr::tr("Open File"),
            m_qrcModel->absolutePath(QString()),
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

    return QLatin1Char(':') + currentPrefix() + QLatin1Char('/') + m_qrcModel->relativePath(m_qrcModel->file(current));
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

void ResourceView::onItemActivated(const QModelIndex &index)
{
    const QString fileName = m_qrcModel->file(index);
    if (fileName.isEmpty())
        return;
    emit itemActivated(fileName);
}

void ResourceView::showContextMenu(const QPoint &pos)
{
    const QModelIndex index = indexAt(pos);
    const QString fileName = m_qrcModel->file(index);
    if (fileName.isEmpty())
        return;
    emit contextMenuShown(mapToGlobal(pos), fileName);
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
    m_treeview(new ResourceView(model, &m_history))
{
    addWidget(m_treeview);
    auto widget = new QWidget;
    addWidget(widget);
    m_treeview->setFrameStyle(QFrame::NoFrame);
    m_treeview->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

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

    connect(m_treeview, &ResourceView::removeItem, this, &QrcEditor::onRemove);
    connect(m_treeview->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &QrcEditor::updateCurrent);
    connect(m_treeview, &ResourceView::itemActivated, this, &QrcEditor::itemActivated);
    connect(m_treeview, &ResourceView::contextMenuShown, this, &QrcEditor::showContextMenu);
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

    updateHistoryControls();
    updateCurrent();
}

QrcEditor::~QrcEditor() = default;

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
    m_treeview->refresh();
}

// Propagates a change of selection in the tree
// to the alias/prefix/language edit controls
void QrcEditor::updateCurrent()
{
    const bool isValid = m_treeview->currentIndex().isValid();
    const bool isPrefix = m_treeview->isPrefix(m_treeview->currentIndex()) && isValid;
    const bool isFile = !isPrefix && isValid;

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

    m_addFilesButton->setEnabled(isValid);
    m_removeButton->setEnabled(isValid);
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
}

bool QrcEditor::resourceDragEnabled() const
{
    return m_treeview->resourceDragEnabled();
}

void QrcEditor::editCurrentItem()
{
    if (m_treeview->selectionModel()->currentIndex().isValid())
        m_treeview->edit(m_treeview->selectionModel()->currentIndex());
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

// Slot for 'Remove' button
void QrcEditor::onRemove()
{
    // Find current item, push and execute command
    const QModelIndex current = m_treeview->currentIndex();
    int afterDeletionArrayIndex = current.row();
    QModelIndex afterDeletionParent = current.parent();
    m_treeview->findSamePlacePostDeletionModelIndex(afterDeletionArrayIndex, afterDeletionParent);
    QUndoCommand * const removeCommand = new RemoveEntryCommand(m_treeview, current);
    m_history.push(removeCommand);
    const QModelIndex afterDeletionModelIndex
            = m_treeview->model()->index(afterDeletionArrayIndex, 0, afterDeletionParent);
    m_treeview->setCurrentIndex(afterDeletionModelIndex);
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
    QStringList fileNames = m_treeview->fileNamesToAdd();
    fileNames = m_treeview->existingFilesSubtracted(prefixArrayIndex, fileNames);
    resolveLocationIssues(fileNames);
    if (fileNames.isEmpty())
        return;
    QUndoCommand * const addFilesCommand = new AddFilesCommand(
            m_treeview, prefixArrayIndex, cursorFileArrayIndex, fileNames);
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

} // ResourceEditor::Internal

#include "qrceditor.moc"
