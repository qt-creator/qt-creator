/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "snippetssettingspage.h"
#include "snippeteditor.h"
#include "isnippetprovider.h"
#include "snippet.h"
#include "snippetscollection.h"
#include "snippetssettings.h"
#include "reuse.h"
#include "ui_snippetssettingspage.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QModelIndex>
#include <QtCore/QAbstractTableModel>
#include <QtCore/QList>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtCore/QHash>
#include <QtGui/QMessageBox>

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
        return tr("Complement");
}

void SnippetsTableModel::load(const QString &groupId)
{
    m_activeGroupId = groupId;
    reset();
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
    m_collection->restoreRemovedSnippets(m_activeGroupId);
    reset();
}

void SnippetsTableModel::resetSnippets()
{
    m_collection->reset(m_activeGroupId);
    reset();
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
    SnippetsSettingsPagePrivate(const QString &id);
    ~SnippetsSettingsPagePrivate() { delete m_model; }

    const QString &id() const { return m_id; }
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

private:
    SnippetEditor *currentEditor() const;
    SnippetEditor *editorAt(int i) const;

    void loadSettings();
    bool settingsChanged() const;
    void writeSettings();

    const QString m_id;
    const QString m_displayName;
    const QString m_settingsPrefix;
    SnippetsTableModel *m_model;
    bool m_snippetsCollectionChanged;
    QString m_keywords;
    SnippetsSettings m_settings;
    Ui::SnippetsSettingsPage m_ui;
};

SnippetsSettingsPagePrivate::SnippetsSettingsPagePrivate(const QString &id) :
    m_id(id),
    m_displayName(tr("Snippets")),
    m_settingsPrefix(QLatin1String("Text")),
    m_model(new SnippetsTableModel(0)),
    m_snippetsCollectionChanged(false)
{}

SnippetEditor *SnippetsSettingsPagePrivate::currentEditor() const
{
    return editorAt(m_ui.snippetsEditorStack->currentIndex());
}

SnippetEditor *SnippetsSettingsPagePrivate::editorAt(int i) const
{
    return static_cast<SnippetEditor *>(m_ui.snippetsEditorStack->widget(i));
}

void SnippetsSettingsPagePrivate::configureUi(QWidget *w)
{
    m_ui.setupUi(w);

    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::instance()->getObjects<ISnippetProvider>();
    foreach (ISnippetProvider *provider, providers) {
        m_ui.groupCombo->addItem(provider->displayName(), provider->groupId());
        SnippetEditor *snippetEditor = new SnippetEditor(w);
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

    connect(m_model, SIGNAL(rowsInserted(QModelIndex, int, int)),
            this, SLOT(selectSnippet(QModelIndex,int)));
    connect(m_model, SIGNAL(rowsInserted(QModelIndex, int, int)),
            this, SLOT(markSnippetsCollection()));
    connect(m_model, SIGNAL(rowsRemoved(QModelIndex, int, int)),
            this, SLOT(markSnippetsCollection()));
    connect(m_model, SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)),
            this, SLOT(selectMovedSnippet(QModelIndex,int,int,QModelIndex,int)));
    connect(m_model, SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)),
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
}

void SnippetsSettingsPagePrivate::apply()
{
    if (settingsChanged())
        writeSettings();

    if (m_snippetsCollectionChanged) {
        SnippetsCollection::instance()->synchronize();
        m_snippetsCollectionChanged = false;
    }
}

void SnippetsSettingsPagePrivate::finish()
{
    if (m_snippetsCollectionChanged) {
        SnippetsCollection::instance()->reload();
        m_snippetsCollectionChanged = false;
    }
}

void SnippetsSettingsPagePrivate::loadSettings()
{
    if (m_ui.groupCombo->count() == 0)
        return;

    if (QSettings *s = Core::ICore::instance()->settings()) {
        m_settings.fromSettings(m_settingsPrefix, s);
        const QString &lastGroupName = m_settings.lastUsedSnippetGroup();
        const int index = m_ui.groupCombo->findText(lastGroupName);
        if (index != -1)
            m_ui.groupCombo->setCurrentIndex(index);
        else
            m_ui.groupCombo->setCurrentIndex(0);
    }
}

void SnippetsSettingsPagePrivate::writeSettings()
{
    if (m_ui.groupCombo->count() == 0)
        return;

    if (QSettings *s = Core::ICore::instance()->settings()) {
        m_settings.setLastUsedSnippetGroup(m_ui.groupCombo->currentText());
        m_settings.toSettings(m_settingsPrefix, s);
    }
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

// SnippetsSettingsPage
SnippetsSettingsPage::SnippetsSettingsPage(const QString &id, QObject *parent) :
    TextEditorOptionsPage(parent),
    m_d(new SnippetsSettingsPagePrivate(id))
{}

SnippetsSettingsPage::~SnippetsSettingsPage()
{
    delete m_d;
}

QString SnippetsSettingsPage::id() const
{
    return m_d->id();
}

QString SnippetsSettingsPage::displayName() const
{
    return m_d->displayName();
}

bool SnippetsSettingsPage::matches(const QString &s) const
{
    return m_d->isKeyword(s);
}

QWidget *SnippetsSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->configureUi(w);
    return w;
}

void SnippetsSettingsPage::apply()
{
    m_d->apply();
}

void SnippetsSettingsPage::finish()
{
    m_d->finish();
}

} // Internal
} // TextEditor

#include "snippetssettingspage.moc"
