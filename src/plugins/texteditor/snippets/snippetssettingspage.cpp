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
#include "../texteditortr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/guiutils.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QAbstractButton>
#include <QAbstractTableModel>
#include <QComboBox>
#include <QItemSelectionModel>
#include <QList>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextStream>

#ifdef WITH_TESTS
#include <QTest>
#endif

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
    void reloadSnippets();

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

void SnippetsTableModel::reloadSnippets()
{
    beginResetModel();
    SnippetsCollection::instance()->reload();
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
    Q_OBJECT

public:
    explicit SnippetsSettingsAspect(AspectContainer *container);

    void addToLayoutImpl(Layouting::Layout &parent) override;
    void apply() override;
    void cancel() override;
    bool isDirty() const override { return m_snippetsCollectionChanged || settingsChanged(); }

    SnippetsTableModel &model() { return m_model; }
    int selectedGroup() const { return m_selectedGroup; }
    void selectGroup(int index);
    void markSnippetsCollection();

signals:
    void selectedGroupChanged(QPrivateSignal);
    void pendingContentRequested(QPrivateSignal);

private:
    void loadSettings();
    bool settingsChanged() const;
    void saveSettings();
    QString selectedGroupName() const;

    SnippetsTableModel m_model;
    bool m_snippetsCollectionChanged = false;
    QString m_lastUsedSnippetGroup;
    int m_selectedGroup = -1;
};

// The widgets of one layout of the aspect. They share the aspect's model and
// selected group, and keep their own snippet selection.
class SnippetsSettingsWidget final : public QWidget
{
public:
    explicit SnippetsSettingsWidget(SnippetsSettingsAspect *aspect);

private:
    void showSelectedGroup();
    void addSnippet();
    void removeSnippet();
    void revertBuiltInSnippet();
    void selectSnippet(const QModelIndex &parent, int row);
    void selectMovedSnippet(const QModelIndex &, int, int, const QModelIndex &, int row);
    void setSnippetContent();
    void updateCurrentSnippetDependent(const QModelIndex &modelIndex = QModelIndex());
    void decorateEditors(const FontSettingsData &fontSettings);

    SnippetEditorWidget *currentEditor() const;
    SnippetEditorWidget *editorAt(int i) const;

    SnippetsSettingsAspect *m_aspect;
    QStackedWidget *m_snippetsEditorStack;
    QComboBox *m_groupCombo;
    TreeView *m_snippetsTable;
    QPushButton *m_revertButton;
};

SnippetsSettingsAspect::SnippetsSettingsAspect(AspectContainer *container)
    : BaseAspect(container)
{
    connect(&m_model, &QAbstractItemModel::rowsInserted,
            this, &SnippetsSettingsAspect::markSnippetsCollection);
    connect(&m_model, &QAbstractItemModel::rowsRemoved,
            this, &SnippetsSettingsAspect::markSnippetsCollection);
    connect(&m_model, &QAbstractItemModel::rowsMoved,
            this, &SnippetsSettingsAspect::markSnippetsCollection);
    connect(&m_model, &QAbstractItemModel::dataChanged,
            this, &SnippetsSettingsAspect::markSnippetsCollection);
    connect(&m_model, &QAbstractItemModel::modelReset,
            this, &SnippetsSettingsAspect::markSnippetsCollection);
}

void SnippetsSettingsAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto widget = new SnippetsSettingsWidget(this);
    registerSubWidget(widget);
    if (m_selectedGroup == -1)
        loadSettings();
    parent.addItem(widget);
}

void SnippetsSettingsAspect::apply()
{
    if (settingsChanged())
        saveSettings();

    emit pendingContentRequested(QPrivateSignal());

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
    if (settingsChanged()) {
        const int index = Utils::indexOf(SnippetProvider::snippetProviders(),
                                         [this](const SnippetProvider &provider) {
            return provider.displayName() == m_lastUsedSnippetGroup;
        });
        selectGroup(index != -1 ? index : 0);
    }
    if (m_snippetsCollectionChanged) {
        m_model.reloadSnippets();
        m_snippetsCollectionChanged = false;
    }
}

void SnippetsSettingsAspect::selectGroup(int index)
{
    if (index == -1)
        return;

    DirtySettingsGuard suppressor;

    m_selectedGroup = index;
    const QList<SnippetProvider> providers = SnippetProvider::snippetProviders();
    if (QTC_GUARD(index < providers.size())) {
        // Showing another group does not change the collection.
        const bool collectionChanged = m_snippetsCollectionChanged;
        m_model.load(providers.at(index).groupId());
        m_snippetsCollectionChanged = collectionChanged;
    }
    emit selectedGroupChanged(QPrivateSignal());
}

QString SnippetsSettingsAspect::selectedGroupName() const
{
    const QList<SnippetProvider> providers = SnippetProvider::snippetProviders();
    if (m_selectedGroup < 0 || m_selectedGroup >= providers.size())
        return {};
    return providers.at(m_selectedGroup).displayName();
}

void SnippetsSettingsAspect::loadSettings()
{
    const QList<SnippetProvider> providers = SnippetProvider::snippetProviders();
    if (providers.isEmpty())
        return;

    m_lastUsedSnippetGroup = Core::ICore::settings()->value(kLastUsedSnippetGroup, QString()).toString();
    const int index = Utils::indexOf(providers, [this](const SnippetProvider &provider) {
        return provider.displayName() == m_lastUsedSnippetGroup;
    });
    selectGroup(index != -1 ? index : 0);
    m_lastUsedSnippetGroup = selectedGroupName();
}

void SnippetsSettingsAspect::saveSettings()
{
    if (m_selectedGroup == -1)
        return;

    m_lastUsedSnippetGroup = selectedGroupName();
    Core::ICore::settings()->setValue(kLastUsedSnippetGroup, m_lastUsedSnippetGroup);
}

bool SnippetsSettingsAspect::settingsChanged() const
{
    return m_selectedGroup != -1 && m_lastUsedSnippetGroup != selectedGroupName();
}

void SnippetsSettingsAspect::markSnippetsCollection()
{
    m_snippetsCollectionChanged = true;
    markSettingsDirty();
}

SnippetsSettingsWidget::SnippetsSettingsWidget(SnippetsSettingsAspect *aspect)
    : m_aspect(aspect)
{
    m_groupCombo = new QComboBox;
    setWheelScrollingWithoutFocusBlocked(m_groupCombo);
    setIgnoreForDirtyHook(m_groupCombo);
    m_snippetsEditorStack = new QStackedWidget;
    for (const SnippetProvider &provider : SnippetProvider::snippetProviders()) {
        m_groupCombo->addItem(provider.displayName(), provider.groupId());
        auto snippetEditor = new SnippetEditorWidget;
        installMarkSettingsDirtyTrigger(snippetEditor);
        SnippetProvider::decorateEditor(snippetEditor, provider.groupId());
        m_snippetsEditorStack->insertWidget(m_groupCombo->count() - 1, snippetEditor);
        connect(snippetEditor, &SnippetEditorWidget::snippetContentChanged,
                this, &SnippetsSettingsWidget::setSnippetContent);
    }

    m_snippetsTable = new TreeView;
    m_snippetsTable->setRootIsDecorated(false);
    m_snippetsTable->setModel(&m_aspect->model());

    m_revertButton = new QPushButton(Tr::tr("Revert Built-in"));
    m_revertButton->setEnabled(false);

    auto snippetSplitter = new QSplitter(Qt::Vertical);
    snippetSplitter->setChildrenCollapsible(false);
    snippetSplitter->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    snippetSplitter->addWidget(m_snippetsTable);
    snippetSplitter->addWidget(m_snippetsEditorStack);

    using namespace Layouting;
    Column {
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
                    onClicked(this, [this] { m_aspect->model().restoreRemovedBuiltInSnippets(); })
                },
                PushButton {
                    text(Tr::tr("Reset All")),
                    onClicked(this, [this] { m_aspect->model().resetSnippets(); })
                },
                st,
            }
        },
        noMargin
    }.attachTo(this);

    SnippetsTableModel *model = &m_aspect->model();
    connect(model, &QAbstractItemModel::rowsInserted,
            this, &SnippetsSettingsWidget::selectSnippet);
    connect(model, &QAbstractItemModel::rowsMoved,
            this, &SnippetsSettingsWidget::selectMovedSnippet);
    connect(model, &QAbstractItemModel::modelReset,
            this, [this] { updateCurrentSnippetDependent(); });

    connect(m_groupCombo, &QComboBox::currentIndexChanged,
            m_aspect, &SnippetsSettingsAspect::selectGroup);
    connect(m_aspect, &SnippetsSettingsAspect::selectedGroupChanged,
            this, &SnippetsSettingsWidget::showSelectedGroup);
    connect(m_aspect, &SnippetsSettingsAspect::pendingContentRequested, this, [this] {
        if (currentEditor()->document()->isModified())
            setSnippetContent();
    });
    connect(m_revertButton, &QAbstractButton::clicked,
            this, &SnippetsSettingsWidget::revertBuiltInSnippet);
    connect(m_snippetsTable->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current) { updateCurrentSnippetDependent(current); });

    connect(&globalFontSettings(), &FontSettings::changed,
            this, [this] { decorateEditors(globalFontSettings().data()); });

    showSelectedGroup();
}

void SnippetsSettingsWidget::showSelectedGroup()
{
    const int index = m_aspect->selectedGroup();
    if (index == -1)
        return;

    DirtySettingsGuard suppressor;
    {
        const QSignalBlocker blocker(m_groupCombo);
        m_groupCombo->setCurrentIndex(index);
    }
    m_snippetsEditorStack->setCurrentIndex(index);
    currentEditor()->clear();
}

SnippetEditorWidget *SnippetsSettingsWidget::currentEditor() const
{
    return editorAt(m_snippetsEditorStack->currentIndex());
}

SnippetEditorWidget *SnippetsSettingsWidget::editorAt(int i) const
{
    return static_cast<SnippetEditorWidget *>(m_snippetsEditorStack->widget(i));
}

void SnippetsSettingsWidget::addSnippet()
{
    const QModelIndex &modelIndex = m_aspect->model().createSnippet();
    selectSnippet(QModelIndex(), modelIndex.row());
    m_snippetsTable->edit(modelIndex);
}

void SnippetsSettingsWidget::removeSnippet()
{
    const QModelIndex &modelIndex = m_snippetsTable->selectionModel()->currentIndex();
    if (!modelIndex.isValid()) {
        QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Error"), Tr::tr("No snippet selected."));
        return;
    }
    m_aspect->model().removeSnippet(modelIndex);
}

void SnippetsSettingsWidget::revertBuiltInSnippet()
{
    m_aspect->model().revertBuitInSnippet(m_snippetsTable->selectionModel()->currentIndex());
}

void SnippetsSettingsWidget::selectSnippet(const QModelIndex &parent, int row)
{
    QModelIndex topLeft = m_aspect->model().index(row, 0, parent);
    QModelIndex bottomRight = m_aspect->model().index(row, 1, parent);
    QItemSelection selection(topLeft, bottomRight);
    m_snippetsTable->selectionModel()->select(selection, QItemSelectionModel::SelectCurrent);
    m_snippetsTable->setCurrentIndex(topLeft);
    m_snippetsTable->scrollTo(topLeft);
}

void SnippetsSettingsWidget::selectMovedSnippet(const QModelIndex &,
                                                int sourceRow,
                                                int,
                                                const QModelIndex &destinationParent,
                                                int destinationRow)
{
    QModelIndex modelIndex;
    if (sourceRow < destinationRow)
        modelIndex = m_aspect->model().index(destinationRow - 1, 0, destinationParent);
    else
        modelIndex = m_aspect->model().index(destinationRow, 0, destinationParent);
    m_snippetsTable->scrollTo(modelIndex);
    currentEditor()->setPlainText(m_aspect->model().snippetAt(modelIndex).content());
}

void SnippetsSettingsWidget::updateCurrentSnippetDependent(const QModelIndex &modelIndex)
{
    DirtySettingsGuard suppressor;
    if (modelIndex.isValid()) {
        const Snippet &snippet = m_aspect->model().snippetAt(modelIndex);
        currentEditor()->setPlainText(snippet.content());
        m_revertButton->setEnabled(snippet.isBuiltIn());
    } else {
        currentEditor()->clear();
        m_revertButton->setEnabled(false);
    }
}

void SnippetsSettingsWidget::setSnippetContent()
{
    const QModelIndex &modelIndex = m_snippetsTable->selectionModel()->currentIndex();
    if (modelIndex.isValid()) {
        m_aspect->model().setSnippetContent(modelIndex, currentEditor()->toPlainText());
        m_aspect->markSnippetsCollection();
    }
}

void SnippetsSettingsWidget::decorateEditors(const FontSettingsData &fontSettings)
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

#ifdef WITH_TESTS
class SnippetsSettingsAspectTest final : public QObject
{
    Q_OBJECT

    static void registerTestGroup()
    {
        // Groups cannot be unregistered, so add an empty one once per process.
        static const bool registered = [] {
            SnippetProvider::registerGroup("TextEditor.SnippetsSettingsAspectTest", "Test");
            return true;
        }();
        Q_UNUSED(registered)
        QVERIFY(SnippetProvider::snippetProviders().size() >= 2);
    }

private slots:
    void testCancelRestoresTheGroup()
    {
        registerTestGroup();
        SnippetsSettingsAspect snippets(nullptr);
        BaseAspect &aspect = snippets;
        const std::unique_ptr<QWidget> first(Layouting::Column{aspect}.emerge());
        QComboBox *firstCombo = first->findChild<QComboBox *>();
        QVERIFY(firstCombo);
        const int originalGroup = firstCombo->currentIndex();

        firstCombo->setCurrentIndex(originalGroup == 0 ? 1 : 0);
        QVERIFY(aspect.isDirty());
        aspect.cancel();

        QVERIFY(!aspect.isDirty());
        QCOMPARE(firstCombo->currentIndex(), originalGroup);
        const std::unique_ptr<QWidget> second(Layouting::Column{aspect}.emerge());
        QComboBox *secondCombo = second->findChild<QComboBox *>();
        QVERIFY(secondCombo);
        QCOMPARE(secondCombo->currentIndex(), originalGroup);
    }

    void testFollowsTheGroupInEveryWidget()
    {
        registerTestGroup();

        SnippetsSettingsAspect snippets(nullptr);
        BaseAspect &aspect = snippets;
        const std::unique_ptr<QWidget> first(Layouting::Column{aspect}.emerge());
        QComboBox *firstCombo = first->findChild<QComboBox *>();
        QVERIFY(firstCombo);
        QVERIFY(!aspect.isDirty());
        const int otherGroup = firstCombo->currentIndex() == 0 ? 1 : 0;

        firstCombo->setCurrentIndex(otherGroup);
        QVERIFY(aspect.isDirty());

        // A layout made later shows the selected group and leaves the others alone.
        const std::unique_ptr<QWidget> second(Layouting::Column{aspect}.emerge());
        QComboBox *secondCombo = second->findChild<QComboBox *>();
        QStackedWidget *secondStack = second->findChild<QStackedWidget *>();
        QVERIFY(secondCombo);
        QVERIFY(secondStack);
        QCOMPARE(firstCombo->currentIndex(), otherGroup);
        QCOMPARE(secondCombo->currentIndex(), otherGroup);
        QCOMPARE(secondStack->currentIndex(), otherGroup);

        secondCombo->setCurrentIndex(1 - otherGroup);
        QCOMPARE(firstCombo->currentIndex(), 1 - otherGroup);
    }
};

QObject *createSnippetsSettingsAspectTest()
{
    return new SnippetsSettingsAspectTest;
}
#endif // WITH_TESTS

} // TextEditor::Internal

#include "snippetssettingspage.moc"
