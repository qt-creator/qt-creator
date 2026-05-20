// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "snippetssettingspage.h"

#include "snippeteditor.h"
#include "snippetprovider.h"
#include "snippet.h"
#include "snippetscollection.h"
#include "../fontsettings.h"
#include "../textdocument.h"
#include "../texteditorconstants.h"
#include "../texteditorsettings.h"
#include "../texteditortr.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/aspects.h>
#include <utils/headerviewstretcher.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>

#include <QAbstractButton>
#include <QAbstractTableModel>
#include <QComboBox>
#include <QItemSelectionModel>
#include <QList>
#include <QMessageBox>
#include <QModelIndex>
#include <QPointer>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextStream>

using namespace Utils;

namespace TextEditor::Internal {

const char kLastUsedSnippetGroup[] = "TextSnippetsSettings/LastUsedSnippetGroup";

// SnippetsTableModel

class SnippetsTableModel final : public QAbstractTableModel
{
public:
    SnippetsTableModel() = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &modelIndex) const override;
    QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &modelIndex, const QVariant &value,
                 int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QStringList groupIds() const;
    void load(const QString &groupId);

    QModelIndex createSnippet();
    QModelIndex insertSnippet(const Snippet &snippet);
    void removeSnippet(const QModelIndex &modelIndex);
    const Snippet &snippetAt(const QModelIndex &modelIndex) const;
    void setSnippetContent(const QModelIndex &modelIndex, const QString &content);
    void revertBuitInSnippet(const QModelIndex &modelIndex);
    void restoreRemovedBuiltInSnippets();
    void resetSnippets();

private:
    void replaceSnippet(const Snippet &snippet, const QModelIndex &modelIndex);

    QString m_activeGroupId;
};

int SnippetsTableModel::rowCount(const QModelIndex &) const
{
    return SnippetsCollection::instance()->totalActiveSnippets(m_activeGroupId);
}

int SnippetsTableModel::columnCount(const QModelIndex &) const
{
    return 2;
}

Qt::ItemFlags SnippetsTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags itemFlags = QAbstractTableModel::flags(index);
    if (index.isValid())
        itemFlags |= Qt::ItemIsEditable;
    return itemFlags;
}

QVariant SnippetsTableModel::data(const QModelIndex &modelIndex, int role) const
{
    if (!modelIndex.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const Snippet &snippet = SnippetsCollection::instance()->snippet(modelIndex.row(), m_activeGroupId);
        if (modelIndex.column() == 0)
            return snippet.trigger();
        else
            return snippet.complement();
    } else {
        return QVariant();
    }
}

bool SnippetsTableModel::setData(const QModelIndex &modelIndex, const QVariant &value, int role)
{
    if (modelIndex.isValid() && role == Qt::EditRole) {
        Snippet snippet(SnippetsCollection::instance()->snippet(modelIndex.row(), m_activeGroupId));
        if (modelIndex.column() == 0) {
            const QString &s = value.toString();
            if (!Snippet::isValidTrigger(s)) {
                QMessageBox::critical(
                    Core::ICore::dialogParent(),
                    Tr::tr("Error"),
                    Tr::tr("Not a valid trigger. A valid trigger can only contain letters, "
                       "numbers, or underscores, where the first character is "
                       "limited to letter or underscore."));
                if (snippet.trigger().isEmpty())
                    removeSnippet(modelIndex);
                return false;
            }
            snippet.setTrigger(s);
        } else {
            snippet.setComplement(value.toString());
        }

        replaceSnippet(snippet, modelIndex);
        return true;
    }
    return false;
}

QVariant SnippetsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    if (section == 0)
        return Tr::tr("Trigger");
    else
        return Tr::tr("Trigger Variant");
}

void SnippetsTableModel::load(const QString &groupId)
{
    beginResetModel();
    m_activeGroupId = groupId;
    endResetModel();
}

QStringList SnippetsTableModel::groupIds() const
{
    return SnippetsCollection::instance()->groupIds();
}

QModelIndex SnippetsTableModel::createSnippet()
{
    Snippet snippet(m_activeGroupId);
    return insertSnippet(snippet);
}

QModelIndex SnippetsTableModel::insertSnippet(const Snippet &snippet)
{
    const SnippetsCollection::Hint &hint = SnippetsCollection::instance()->computeInsertionHint(snippet);
    beginInsertRows(QModelIndex(), hint.index(), hint.index());
    SnippetsCollection::instance()->insertSnippet(snippet, hint);
    endInsertRows();

    return index(hint.index(), 0);
}

void SnippetsTableModel::removeSnippet(const QModelIndex &modelIndex)
{
    beginRemoveRows(QModelIndex(), modelIndex.row(), modelIndex.row());
    SnippetsCollection::instance()->removeSnippet(modelIndex.row(), m_activeGroupId);
    endRemoveRows();
}

const Snippet &SnippetsTableModel::snippetAt(const QModelIndex &modelIndex) const
{
    return SnippetsCollection::instance()->snippet(modelIndex.row(), m_activeGroupId);
}

void SnippetsTableModel::setSnippetContent(const QModelIndex &modelIndex, const QString &content)
{
    SnippetsCollection::instance()->setSnippetContent(modelIndex.row(), m_activeGroupId, content);
}

void SnippetsTableModel::revertBuitInSnippet(const QModelIndex &modelIndex)
{
    const Snippet &snippet = SnippetsCollection::instance()->revertedSnippet(modelIndex.row(), m_activeGroupId);
    if (snippet.id().isEmpty()) {
        QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Error"),
                              Tr::tr("Error reverting snippet."));
        return;
    }
    replaceSnippet(snippet, modelIndex);
}

void SnippetsTableModel::restoreRemovedBuiltInSnippets()
{
    beginResetModel();
    SnippetsCollection::instance()->restoreRemovedSnippets(m_activeGroupId);
    endResetModel();
}

void SnippetsTableModel::resetSnippets()
{
    beginResetModel();
    SnippetsCollection::instance()->reset(m_activeGroupId);
    endResetModel();
}

void SnippetsTableModel::replaceSnippet(const Snippet &snippet, const QModelIndex &modelIndex)
{
    const int row = modelIndex.row();
    const SnippetsCollection::Hint &hint =
        SnippetsCollection::instance()->computeReplacementHint(row, snippet);
    if (modelIndex.row() == hint.index()) {
        SnippetsCollection::instance()->replaceSnippet(row, snippet, hint);
        if (modelIndex.column() == 0)
            emit dataChanged(modelIndex, modelIndex.sibling(row, 1));
        else
            emit dataChanged(modelIndex.sibling(row, 0), modelIndex);
    } else {
        if (row < hint.index())
            // Rows will be moved down.
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), hint.index() + 1);
        else
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), hint.index());
        SnippetsCollection::instance()->replaceSnippet(row, snippet, hint);
        endMoveRows();
    }
}

class SnippetsSettingsAspect final : public BaseAspect
{
public:
    using BaseAspect::BaseAspect;

    void addToLayoutImpl(Layouting::Layout &parent) override;
    void apply() override;
    void cancel() override;
    bool isDirty() const override { return m_snippetsCollectionChanged || settingsChanged(); }

private:
    void loadSnippetGroup(int index);
    void markSnippetsCollection();
    void addSnippet();
    void removeSnippet();
    void revertBuiltInSnippet();
    void restoreRemovedBuiltInSnippets();
    void resetAllSnippets();
    void selectSnippet(const QModelIndex &parent, int row);
    void selectMovedSnippet(const QModelIndex &, int, int, const QModelIndex &, int row);
    void setSnippetContent();
    void updateCurrentSnippetDependent(const QModelIndex &modelIndex = QModelIndex());
    void decorateEditors(const FontSettings &fontSettings);

    SnippetEditorWidget *currentEditor() const;
    SnippetEditorWidget *editorAt(int i) const;

    void loadSettings();
    bool settingsChanged() const;
    void saveSettings();

    SnippetsTableModel m_model;
    bool m_snippetsCollectionChanged = false;
    QString m_lastUsedSnippetGroup;

    QPointer<QStackedWidget> m_snippetsEditorStack;
    QPointer<QComboBox> m_groupCombo;
    QPointer<TreeView> m_snippetsTable;
    QPointer<QPushButton> m_revertButton;
};

void SnippetsSettingsAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    m_groupCombo = createSubWidget<QComboBox>();
    setIgnoreForDirtyHook(m_groupCombo);
    m_snippetsEditorStack = createSubWidget<QStackedWidget>();
    for (const SnippetProvider &provider : SnippetProvider::snippetProviders()) {
        m_groupCombo->addItem(provider.displayName(), provider.groupId());
        auto snippetEditor = new SnippetEditorWidget;
        installMarkSettingsDirtyTrigger(snippetEditor);
        SnippetProvider::decorateEditor(snippetEditor, provider.groupId());
        m_snippetsEditorStack->insertWidget(m_groupCombo->count() - 1, snippetEditor);
        connect(snippetEditor, &SnippetEditorWidget::snippetContentChanged,
                this, &SnippetsSettingsAspect::setSnippetContent);
    }

    m_snippetsTable = createSubWidget<TreeView>();
    m_snippetsTable->setRootIsDecorated(false);
    m_snippetsTable->setModel(&m_model);

    m_revertButton = createSubWidget<QPushButton>(Tr::tr("Revert Built-in"));
    m_revertButton->setEnabled(false);

    auto snippetSplitter = new QSplitter(Qt::Vertical);
    snippetSplitter->setChildrenCollapsible(false);
    snippetSplitter->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    snippetSplitter->addWidget(m_snippetsTable);
    snippetSplitter->addWidget(m_snippetsEditorStack);

    using namespace Layouting;
    parent.addItem(Column {
        Row { Tr::tr("Group:"), m_groupCombo, st },
        Row {
            snippetSplitter,
            Column {
                PushButton {
                    text(Tr::tr("Add")),
                    onClicked(this, [this] { addSnippet(); })
                },
                PushButton {
                    text(Tr::tr("Remove")),
                    onClicked(this, [this] { removeSnippet(); })
                },
                m_revertButton,
                PushButton {
                    text(Tr::tr("Restore Removed Built-ins")),
                    onClicked(this, [this] { restoreRemovedBuiltInSnippets(); })
                },
                PushButton {
                    text(Tr::tr("Reset All")),
                    onClicked(this, [this] { resetAllSnippets(); })
                },
                st,
            }
        }
    });

    loadSettings();
    loadSnippetGroup(m_groupCombo->currentIndex());

    connect(&m_model, &QAbstractItemModel::rowsInserted,
            this, &SnippetsSettingsAspect::selectSnippet);
    connect(&m_model, &QAbstractItemModel::rowsInserted,
            this, &SnippetsSettingsAspect::markSnippetsCollection);
    connect(&m_model, &QAbstractItemModel::rowsRemoved,
            this, &SnippetsSettingsAspect::markSnippetsCollection);
    connect(&m_model, &QAbstractItemModel::rowsMoved,
            this, &SnippetsSettingsAspect::selectMovedSnippet);
    connect(&m_model, &QAbstractItemModel::rowsMoved,
            this, &SnippetsSettingsAspect::markSnippetsCollection);
    connect(&m_model, &QAbstractItemModel::dataChanged,
            this, &SnippetsSettingsAspect::markSnippetsCollection);
    connect(&m_model, &QAbstractItemModel::modelReset,
            this, [this] { updateCurrentSnippetDependent(); });
    connect(&m_model, &QAbstractItemModel::modelReset,
            this, &SnippetsSettingsAspect::markSnippetsCollection);

    connect(m_groupCombo, &QComboBox::currentIndexChanged,
            this, &SnippetsSettingsAspect::loadSnippetGroup);
    connect(m_revertButton, &QAbstractButton::clicked,
            this, &SnippetsSettingsAspect::revertBuiltInSnippet);
    connect(m_snippetsTable->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &SnippetsSettingsAspect::updateCurrentSnippetDependent);

    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
            this, &SnippetsSettingsAspect::decorateEditors);
}

SnippetEditorWidget *SnippetsSettingsAspect::currentEditor() const
{
    return editorAt(m_snippetsEditorStack->currentIndex());
}

SnippetEditorWidget *SnippetsSettingsAspect::editorAt(int i) const
{
    return static_cast<SnippetEditorWidget *>(m_snippetsEditorStack->widget(i));
}

void SnippetsSettingsAspect::apply()
{
    if (settingsChanged())
        saveSettings();

    if (currentEditor()->document()->isModified())
        setSnippetContent();

    if (m_snippetsCollectionChanged) {
        if (const Result<> res = SnippetsCollection::instance()->synchronize()) {
            m_snippetsCollectionChanged = false;
        } else {
            QMessageBox::critical(Core::ICore::dialogParent(),
                                  Tr::tr("Error While Saving Snippet Collection"), res.error());
        }
    }
}

void SnippetsSettingsAspect::cancel()
{
    if (m_snippetsCollectionChanged) {
        SnippetsCollection::instance()->reload();
        m_snippetsCollectionChanged = false;
    }
}

void SnippetsSettingsAspect::loadSettings()
{
    if (m_groupCombo->count() == 0)
        return;

    m_lastUsedSnippetGroup = Core::ICore::settings()->value(kLastUsedSnippetGroup, QString()).toString();
    const int index = m_groupCombo->findText(m_lastUsedSnippetGroup);
    if (index != -1)
        m_groupCombo->setCurrentIndex(index);
    else
        m_groupCombo->setCurrentIndex(0);
}

void SnippetsSettingsAspect::saveSettings()
{
    if (m_groupCombo->count() == 0)
        return;

    m_lastUsedSnippetGroup = m_groupCombo->currentText();
    Core::ICore::settings()->setValue(kLastUsedSnippetGroup, m_lastUsedSnippetGroup);
}

bool SnippetsSettingsAspect::settingsChanged() const
{
    return m_groupCombo && m_lastUsedSnippetGroup != m_groupCombo->currentText();
}

void SnippetsSettingsAspect::loadSnippetGroup(int index)
{
    if (index == -1)
        return;

    DirtySettingsGuard suppressor;

    m_snippetsEditorStack->setCurrentIndex(index);
    currentEditor()->clear();
    m_model.load(m_groupCombo->itemData(index).toString());
}

void SnippetsSettingsAspect::markSnippetsCollection()
{
    m_snippetsCollectionChanged = true;
    markSettingsDirty();
}

void SnippetsSettingsAspect::addSnippet()
{
    const QModelIndex &modelIndex = m_model.createSnippet();
    selectSnippet(QModelIndex(), modelIndex.row());
    m_snippetsTable->edit(modelIndex);
}

void SnippetsSettingsAspect::removeSnippet()
{
    const QModelIndex &modelIndex = m_snippetsTable->selectionModel()->currentIndex();
    if (!modelIndex.isValid()) {
        QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Error"), Tr::tr("No snippet selected."));
        return;
    }
    m_model.removeSnippet(modelIndex);
}

void SnippetsSettingsAspect::restoreRemovedBuiltInSnippets()
{
    m_model.restoreRemovedBuiltInSnippets();
}

void SnippetsSettingsAspect::revertBuiltInSnippet()
{
    m_model.revertBuitInSnippet(m_snippetsTable->selectionModel()->currentIndex());
}

void SnippetsSettingsAspect::resetAllSnippets()
{
    m_model.resetSnippets();
}

void SnippetsSettingsAspect::selectSnippet(const QModelIndex &parent, int row)
{
    QModelIndex topLeft = m_model.index(row, 0, parent);
    QModelIndex bottomRight = m_model.index(row, 1, parent);
    QItemSelection selection(topLeft, bottomRight);
    m_snippetsTable->selectionModel()->select(selection, QItemSelectionModel::SelectCurrent);
    m_snippetsTable->setCurrentIndex(topLeft);
    m_snippetsTable->scrollTo(topLeft);
}

void SnippetsSettingsAspect::selectMovedSnippet(const QModelIndex &,
                                                     int sourceRow,
                                                     int,
                                                     const QModelIndex &destinationParent,
                                                     int destinationRow)
{
    QModelIndex modelIndex;
    if (sourceRow < destinationRow)
        modelIndex = m_model.index(destinationRow - 1, 0, destinationParent);
    else
        modelIndex = m_model.index(destinationRow, 0, destinationParent);
    m_snippetsTable->scrollTo(modelIndex);
    currentEditor()->setPlainText(m_model.snippetAt(modelIndex).content());
}

void SnippetsSettingsAspect::updateCurrentSnippetDependent(const QModelIndex &modelIndex)
{
    DirtySettingsGuard suppressor;
    if (modelIndex.isValid()) {
        const Snippet &snippet = m_model.snippetAt(modelIndex);
        currentEditor()->setPlainText(snippet.content());
        m_revertButton->setEnabled(snippet.isBuiltIn());
    } else {
        currentEditor()->clear();
        m_revertButton->setEnabled(false);
    }
}

void SnippetsSettingsAspect::setSnippetContent()
{
    const QModelIndex &modelIndex = m_snippetsTable->selectionModel()->currentIndex();
    if (modelIndex.isValid()) {
        m_model.setSnippetContent(modelIndex, currentEditor()->toPlainText());
        markSnippetsCollection();
    }
}

void SnippetsSettingsAspect::decorateEditors(const FontSettings &fontSettings)
{
    for (int i = 0; i < m_groupCombo->count(); ++i) {
        SnippetEditorWidget *snippetEditor = editorAt(i);
        snippetEditor->textDocument()->setFontSettings(fontSettings);
        const QString &id = m_groupCombo->itemData(i).toString();
        // This list should be quite short... Re-iterating over it is ok.
        SnippetProvider::decorateEditor(snippetEditor, id);
    }
}

// SnippetsSettingsPage

class SnippetsSettings final : public AspectContainer
{
public:
    SnippetsSettings()
    {
        setAutoApply(false);
        setLayouter([this] {
            using namespace Layouting;
            return Column { &m_snippets, noMargin };
        });
    }

private:
    SnippetsSettingsAspect m_snippets{this};
};

class SnippetsSettingsPage final : public Core::IOptionsPage
{
public:
    SnippetsSettingsPage()
    {
        setId(Constants::TEXT_EDITOR_SNIPPETS_SETTINGS);
        setDisplayName(Tr::tr("Snippets"));
        setCategory(Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
        setSettingsProvider([this] { return &m_snippets; });
    }

private:
    SnippetsSettings m_snippets;
};

void setupSnippetsSettings()
{
    static SnippetsSettingsPage theSnippetsSettingsPage;
}

} // TextEditor::Internal
