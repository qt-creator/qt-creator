/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "snippetssettingspage.h"
#include "snippeteditor.h"
#include "isnippetprovider.h"
#include "snippet.h"
#include "snippetscollection.h"
#include "snippetssettings.h"
#include "reuse.h"
#include "ui_snippetssettingspage.h"

#include <coreplugin/icore.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <extensionsystem/pluginmanager.h>

#include <QModelIndex>
#include <QAbstractTableModel>
#include <QList>
#include <QSettings>
#include <QTextStream>
#include <QHash>
#include <QMessageBox>

namespace TextEditor {
namespace Internal {

// SnippetsTableModel
class SnippetsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SnippetsTableModel(QObject *parent);
    virtual ~SnippetsTableModel() {}

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual Qt::ItemFlags flags(const QModelIndex &modelIndex) const;
    virtual QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex &modelIndex, const QVariant &value,
                         int role = Qt::EditRole);
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

    QList<QString> groupIds() const;
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
    static bool isValidTrigger(const QString &s);

    SnippetsCollection* m_collection;
    QString m_activeGroupId;
};

SnippetsTableModel::SnippetsTableModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_collection(SnippetsCollection::instance())
{}

int SnippetsTableModel::rowCount(const QModelIndex &) const
{
    return m_collection->totalActiveSnippets(m_activeGroupId);
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
        const Snippet &snippet = m_collection->snippet(modelIndex.row(), m_activeGroupId);
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
        Snippet snippet(m_collection->snippet(modelIndex.row(), m_activeGroupId));
        if (modelIndex.column() == 0) {
            const QString &s = value.toString();
            if (!isValidTrigger(s)) {
                QMessageBox::critical(0, tr("Error"), tr("Not a valid trigger."));
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
        return tr("Trigger");
    else
        return tr("Trigger Variant");
}

void SnippetsTableModel::load(const QString &groupId)
{
    beginResetModel();
    m_activeGroupId = groupId;
    endResetModel();
}

QList<QString> SnippetsTableModel::groupIds() const
{
    return m_collection->groupIds();
}

QModelIndex SnippetsTableModel::createSnippet()
{
    Snippet snippet(m_activeGroupId);
    return insertSnippet(snippet);
}

QModelIndex SnippetsTableModel::insertSnippet(const Snippet &snippet)
{
    const SnippetsCollection::Hint &hint = m_collection->computeInsertionHint(snippet);
    beginInsertRows(QModelIndex(), hint.index(), hint.index());
    m_collection->insertSnippet(snippet, hint);
    endInsertRows();

    return index(hint.index(), 0);
}

void SnippetsTableModel::removeSnippet(const QModelIndex &modelIndex)
{
    beginRemoveRows(QModelIndex(), modelIndex.row(), modelIndex.row());
    m_collection->removeSnippet(modelIndex.row(), m_activeGroupId);
    endRemoveRows();
}

const Snippet &SnippetsTableModel::snippetAt(const QModelIndex &modelIndex) const
{
    return m_collection->snippet(modelIndex.row(), m_activeGroupId);
}

void SnippetsTableModel::setSnippetContent(const QModelIndex &modelIndex, const QString &content)
{
    m_collection->setSnippetContent(modelIndex.row(), m_activeGroupId, content);
}

void SnippetsTableModel::revertBuitInSnippet(const QModelIndex &modelIndex)
{
    const Snippet &snippet = m_collection->revertedSnippet(modelIndex.row(), m_activeGroupId);
    if (snippet.id().isEmpty()) {
        QMessageBox::critical(0, tr("Error"), tr("Error reverting snippet."));
        return;
    }
    replaceSnippet(snippet, modelIndex);
}

void SnippetsTableModel::restoreRemovedBuiltInSnippets()
{
    beginResetModel();
    m_collection->restoreRemovedSnippets(m_activeGroupId);
    endResetModel();
}

void SnippetsTableModel::resetSnippets()
{
    beginResetModel();
    m_collection->reset(m_activeGroupId);
    endResetModel();
}

void SnippetsTableModel::replaceSnippet(const Snippet &snippet, const QModelIndex &modelIndex)
{
    const int row = modelIndex.row();
    const SnippetsCollection::Hint &hint =
        m_collection->computeReplacementHint(row, snippet);
    if (modelIndex.row() == hint.index()) {
        m_collection->replaceSnippet(row, snippet, hint);
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
        m_collection->replaceSnippet(row, snippet, hint);
        endMoveRows();
    }
}

bool SnippetsTableModel::isValidTrigger(const QString &s)
{
    if (s.isEmpty())
        return false;
    for (int i = 0; i < s.length(); ++i)
        if (!s.at(i).isLetter())
            return false;
    return true;
}

// SnippetsSettingsPagePrivate
class SnippetsSettingsPagePrivate : public QObject
{
    Q_OBJECT
public:
    SnippetsSettingsPagePrivate(Core::Id id);
    ~SnippetsSettingsPagePrivate() { delete m_model; }

    Core::Id id() const { return m_id; }
    const QString &displayName() const { return m_displayName; }
    bool isKeyword(const QString &s) const { return m_keywords.contains(s, Qt::CaseInsensitive); }
    void configureUi(QWidget *parent);

    void apply();
    void finish();

private slots:
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
    void decorateEditors(const TextEditor::FontSettings &fontSettings);

private:
    SnippetEditorWidget *currentEditor() const;
    SnippetEditorWidget *editorAt(int i) const;

    void loadSettings();
    bool settingsChanged() const;
    void writeSettings();

    const Core::Id m_id;
    const QString m_displayName;
    const QString m_settingsPrefix;
    SnippetsTableModel *m_model;
    bool m_snippetsCollectionChanged;
    QString m_keywords;
    SnippetsSettings m_settings;
    Ui::SnippetsSettingsPage m_ui;
};

SnippetsSettingsPagePrivate::SnippetsSettingsPagePrivate(Core::Id id) :
    m_id(id),
    m_displayName(tr("Snippets")),
    m_settingsPrefix(QLatin1String("Text")),
    m_model(new SnippetsTableModel(0)),
    m_snippetsCollectionChanged(false)
{}

SnippetEditorWidget *SnippetsSettingsPagePrivate::currentEditor() const
{
    return editorAt(m_ui.snippetsEditorStack->currentIndex());
}

SnippetEditorWidget *SnippetsSettingsPagePrivate::editorAt(int i) const
{
    return static_cast<SnippetEditorWidget *>(m_ui.snippetsEditorStack->widget(i));
}

void SnippetsSettingsPagePrivate::configureUi(QWidget *w)
{
    m_ui.setupUi(w);

    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::getObjects<ISnippetProvider>();
    foreach (ISnippetProvider *provider, providers) {
        m_ui.groupCombo->addItem(provider->displayName(), provider->groupId());
        SnippetEditorWidget *snippetEditor = new SnippetEditorWidget(w);
        snippetEditor->setFontSettings(TextEditorSettings::instance()->fontSettings());
        provider->decorateEditor(snippetEditor);
        m_ui.snippetsEditorStack->insertWidget(m_ui.groupCombo->count() - 1, snippetEditor);
        connect(snippetEditor, SIGNAL(snippetContentChanged()), this, SLOT(setSnippetContent()));
    }

    m_ui.snippetsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui.snippetsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui.snippetsTable->horizontalHeader()->setStretchLastSection(true);
    m_ui.snippetsTable->horizontalHeader()->setHighlightSections(false);
    m_ui.snippetsTable->verticalHeader()->setVisible(false);
    m_ui.snippetsTable->verticalHeader()->setDefaultSectionSize(20);
    m_ui.snippetsTable->setModel(m_model);

    m_ui.revertButton->setEnabled(false);

    QTextStream(&m_keywords) << m_displayName;

    loadSettings();
    loadSnippetGroup(m_ui.groupCombo->currentIndex());

    connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(selectSnippet(QModelIndex,int)));
    connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(markSnippetsCollection()));
    connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(markSnippetsCollection()));
    connect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            this, SLOT(selectMovedSnippet(QModelIndex,int,int,QModelIndex,int)));
    connect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            this, SLOT(markSnippetsCollection()));
    connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(markSnippetsCollection()));
    connect(m_model, SIGNAL(modelReset()), this, SLOT(updateCurrentSnippetDependent()));
    connect(m_model, SIGNAL(modelReset()), this, SLOT(markSnippetsCollection()));

    connect(m_ui.groupCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(loadSnippetGroup(int)));
    connect(m_ui.addButton, SIGNAL(clicked()), this, SLOT(addSnippet()));
    connect(m_ui.removeButton, SIGNAL(clicked()), this, SLOT(removeSnippet()));
    connect(m_ui.resetAllButton, SIGNAL(clicked()), this, SLOT(resetAllSnippets()));
    connect(m_ui.restoreRemovedButton, SIGNAL(clicked()),
            this, SLOT(restoreRemovedBuiltInSnippets()));
    connect(m_ui.revertButton, SIGNAL(clicked()), this, SLOT(revertBuiltInSnippet()));
    connect(m_ui.snippetsTable->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateCurrentSnippetDependent(QModelIndex)));

    connect(TextEditorSettings::instance(), SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            this, SLOT(decorateEditors(TextEditor::FontSettings)));
}

void SnippetsSettingsPagePrivate::apply()
{
    if (settingsChanged())
        writeSettings();

    if (currentEditor()->document()->isModified())
        setSnippetContent();

    if (m_snippetsCollectionChanged) {
        QString errorString;
        if (SnippetsCollection::instance()->synchronize(&errorString))
            m_snippetsCollectionChanged = false;
        else
            QMessageBox::critical(Core::ICore::mainWindow(),
                    tr("Error While Saving Snippet Collection"), errorString);
    }
}

void SnippetsSettingsPagePrivate::finish()
{
    if (m_snippetsCollectionChanged) {
        SnippetsCollection::instance()->reload();
        m_snippetsCollectionChanged = false;
    }

    disconnect(TextEditorSettings::instance(), 0, this, 0);
}

void SnippetsSettingsPagePrivate::loadSettings()
{
    if (m_ui.groupCombo->count() == 0)
        return;

    m_settings.fromSettings(m_settingsPrefix, Core::ICore::settings());
    const QString &lastGroupName = m_settings.lastUsedSnippetGroup();
    const int index = m_ui.groupCombo->findText(lastGroupName);
    if (index != -1)
        m_ui.groupCombo->setCurrentIndex(index);
    else
        m_ui.groupCombo->setCurrentIndex(0);
}

void SnippetsSettingsPagePrivate::writeSettings()
{
    if (m_ui.groupCombo->count() == 0)
        return;

    m_settings.setLastUsedSnippetGroup(m_ui.groupCombo->currentText());
    m_settings.toSettings(m_settingsPrefix, Core::ICore::settings());
}

bool SnippetsSettingsPagePrivate::settingsChanged() const
{
    if (m_settings.lastUsedSnippetGroup() != m_ui.groupCombo->currentText())
        return true;
    return false;
}

void SnippetsSettingsPagePrivate::loadSnippetGroup(int index)
{
    if (index == -1)
        return;

    m_ui.snippetsEditorStack->setCurrentIndex(index);
    currentEditor()->clear();
    m_model->load(m_ui.groupCombo->itemData(index).toString());
}

void SnippetsSettingsPagePrivate::markSnippetsCollection()
{
    if (!m_snippetsCollectionChanged)
        m_snippetsCollectionChanged = true;
}

void SnippetsSettingsPagePrivate::addSnippet()
{
    const QModelIndex &modelIndex = m_model->createSnippet();
    selectSnippet(QModelIndex(), modelIndex.row());
    m_ui.snippetsTable->edit(modelIndex);
}

void SnippetsSettingsPagePrivate::removeSnippet()
{
    const QModelIndex &modelIndex = m_ui.snippetsTable->selectionModel()->currentIndex();
    if (!modelIndex.isValid()) {
        QMessageBox::critical(0, tr("Error"), tr("No snippet selected."));
        return;
    }
    m_model->removeSnippet(modelIndex);
}

void SnippetsSettingsPagePrivate::restoreRemovedBuiltInSnippets()
{
    m_model->restoreRemovedBuiltInSnippets();
}

void SnippetsSettingsPagePrivate::revertBuiltInSnippet()
{
    m_model->revertBuitInSnippet(m_ui.snippetsTable->selectionModel()->currentIndex());
}

void SnippetsSettingsPagePrivate::resetAllSnippets()
{
    m_model->resetSnippets();
}

void SnippetsSettingsPagePrivate::selectSnippet(const QModelIndex &parent, int row)
{
    QModelIndex topLeft = m_model->index(row, 0, parent);
    QModelIndex bottomRight = m_model->index(row, 1, parent);
    QItemSelection selection(topLeft, bottomRight);
    m_ui.snippetsTable->selectionModel()->select(selection, QItemSelectionModel::SelectCurrent);
    m_ui.snippetsTable->setCurrentIndex(topLeft);
    m_ui.snippetsTable->scrollTo(topLeft);
}

void SnippetsSettingsPagePrivate::selectMovedSnippet(const QModelIndex &,
                                                     int sourceRow,
                                                     int,
                                                     const QModelIndex &destinationParent,
                                                     int destinationRow)
{
    QModelIndex modelIndex;
    if (sourceRow < destinationRow)
        modelIndex = m_model->index(destinationRow - 1, 0, destinationParent);
    else
        modelIndex = m_model->index(destinationRow, 0, destinationParent);
    m_ui.snippetsTable->scrollTo(modelIndex);
    currentEditor()->setPlainText(m_model->snippetAt(modelIndex).content());
}

void SnippetsSettingsPagePrivate::updateCurrentSnippetDependent(const QModelIndex &modelIndex)
{
    if (modelIndex.isValid()) {
        const Snippet &snippet = m_model->snippetAt(modelIndex);
        currentEditor()->setPlainText(snippet.content());
        m_ui.revertButton->setEnabled(snippet.isBuiltIn());
    } else {
        currentEditor()->clear();
        m_ui.revertButton->setEnabled(false);
    }
}

void SnippetsSettingsPagePrivate::setSnippetContent()
{
    const QModelIndex &modelIndex = m_ui.snippetsTable->selectionModel()->currentIndex();
    if (modelIndex.isValid()) {
        m_model->setSnippetContent(modelIndex, currentEditor()->toPlainText());
        markSnippetsCollection();
    }
}

void SnippetsSettingsPagePrivate::decorateEditors(const TextEditor::FontSettings &fontSettings)
{
    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::getObjects<ISnippetProvider>();
    for (int i = 0; i < m_ui.groupCombo->count(); ++i) {
        SnippetEditorWidget *snippetEditor = editorAt(i);
        snippetEditor->setFontSettings(fontSettings);
        const QString &id = m_ui.groupCombo->itemData(i).toString();
        // This list should be quite short... Re-iterating over it is ok.
        foreach (const ISnippetProvider *provider, providers) {
            if (provider->groupId() == id)
                provider->decorateEditor(snippetEditor);
        }
    }
}

// SnippetsSettingsPage
SnippetsSettingsPage::SnippetsSettingsPage(Core::Id id, QObject *parent) :
    TextEditorOptionsPage(parent),
    d(new SnippetsSettingsPagePrivate(id))
{
    setId(d->id());
    setDisplayName(d->displayName());
}

SnippetsSettingsPage::~SnippetsSettingsPage()
{
    delete d;
}

bool SnippetsSettingsPage::matches(const QString &s) const
{
    return d->isKeyword(s);
}

QWidget *SnippetsSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    d->configureUi(w);
    return w;
}

void SnippetsSettingsPage::apply()
{
    d->apply();
}

void SnippetsSettingsPage::finish()
{
    d->finish();
}

} // Internal
} // TextEditor

#include "snippetssettingspage.moc"
