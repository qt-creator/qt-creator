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
#include "snippetsmanager.h"
#include "snippeteditor.h"
#include "isnippeteditordecorator.h"
#include "snippet.h"
#include "snippetscollection.h"
#include "snippetssettings.h"
#include "reuse.h"
#include "ui_snippetssettingspage.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QModelIndex>
#include <QtCore/QAbstractTableModel>
#include <QtCore/QList>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
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

    void load(Snippet::Group group);

    QModelIndex createSnippet();
    QModelIndex insertSnippet(const Snippet &snippet);
    void removeSnippet(const QModelIndex &modelIndex);
    const Snippet &snippetAt(const QModelIndex &modelIndex) const;
    void setSnippetContent(const QModelIndex &modelIndex, const QString &content);

private:
    static bool isValidTrigger(const QString &s);

    Snippet::Group m_activeGroup;
    QSharedPointer<SnippetsCollection> m_collection;
};

SnippetsTableModel::SnippetsTableModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_activeGroup(Snippet::Cpp),
    m_collection(SnippetsManager::instance()->snippetsCollection())
{}

int SnippetsTableModel::rowCount(const QModelIndex &) const
{
    if (m_collection.isNull())
        return 0;
    return m_collection->totalActiveSnippets(m_activeGroup);
}

int SnippetsTableModel::columnCount(const QModelIndex &) const
{
    if (m_collection.isNull())
        return 0;
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
        const Snippet &snippet = m_collection->snippet(modelIndex.row(), m_activeGroup);
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
        const int row = modelIndex.row();
        Snippet snippet(m_collection->snippet(row, m_activeGroup));
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

        const SnippetsCollection::Hint &hint =
            m_collection->computeReplacementHint(row, snippet, m_activeGroup);
        if (modelIndex.row() == hint.index()) {
            m_collection->replaceSnippet(row, snippet, m_activeGroup, hint);
            emit dataChanged(modelIndex, modelIndex);
        } else {
            if (row < hint.index())
                // Rows will be moved down.
                beginMoveRows(QModelIndex(), row, row, QModelIndex(), hint.index() + 1);
            else
                beginMoveRows(QModelIndex(), row, row, QModelIndex(), hint.index());
            m_collection->replaceSnippet(row, snippet, m_activeGroup, hint);
            endMoveRows();
        }

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

void SnippetsTableModel::load(Snippet::Group group)
{
    m_activeGroup = group;
    reset();
}

QModelIndex SnippetsTableModel::createSnippet()
{
    Snippet snippet;
    snippet.setGroup(m_activeGroup);
    return insertSnippet(snippet);
}

QModelIndex SnippetsTableModel::insertSnippet(const Snippet &snippet)
{
    const SnippetsCollection::Hint &hint =
        m_collection->computeInsertionHint(snippet, m_activeGroup);
    beginInsertRows(QModelIndex(), hint.index(), hint.index());
    m_collection->insertSnippet(snippet, m_activeGroup, hint);
    endInsertRows();

    return index(hint.index(), 0);
}

void SnippetsTableModel::removeSnippet(const QModelIndex &modelIndex)
{
    beginRemoveRows(QModelIndex(), modelIndex.row(), modelIndex.row());
    m_collection->removeSnippet(modelIndex.row(), m_activeGroup);
    endRemoveRows();
}

const Snippet &SnippetsTableModel::snippetAt(const QModelIndex &modelIndex) const
{
    return m_collection->snippet(modelIndex.row(), m_activeGroup);
}

void SnippetsTableModel::setSnippetContent(const QModelIndex &modelIndex, const QString &content)
{
    m_collection->setSnippetContent(modelIndex.row(), m_activeGroup, content);
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
    void selectSnippet(const QModelIndex &parent, int row);
    void selectMovedSnippet(const QModelIndex &, int, int, const QModelIndex &, int row);
    void previewSnippet(const QModelIndex &modelIndex);
    void updateSnippetContent();

private:
    SnippetEditor *currentEditor() const;
    SnippetEditor *editorAt(int i) const;

    static void decorateEditor(SnippetEditor *editor, Snippet::Group group);

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

    m_ui.groupCombo->insertItem(Snippet::Cpp, fromSnippetGroup(Snippet::Cpp));
    m_ui.groupCombo->insertItem(Snippet::Qml, fromSnippetGroup(Snippet::Qml));
    m_ui.groupCombo->insertItem(Snippet::PlainText, fromSnippetGroup(Snippet::PlainText));

    m_ui.snippetsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui.snippetsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui.snippetsTable->horizontalHeader()->setStretchLastSection(true);
    m_ui.snippetsTable->horizontalHeader()->setHighlightSections(false);
    m_ui.snippetsTable->verticalHeader()->setVisible(false);
    m_ui.snippetsTable->verticalHeader()->setDefaultSectionSize(20);
    m_ui.snippetsTable->setModel(m_model);

    m_ui.snippetsEditorStack->insertWidget(Snippet::Cpp, new SnippetEditor(w));
    m_ui.snippetsEditorStack->insertWidget(Snippet::Qml, new SnippetEditor(w));
    m_ui.snippetsEditorStack->insertWidget(Snippet::PlainText, new SnippetEditor(w));
    decorateEditor(editorAt(Snippet::Cpp), Snippet::Cpp);
    decorateEditor(editorAt(Snippet::Qml), Snippet::Qml);
    decorateEditor(editorAt(Snippet::PlainText), Snippet::PlainText);

    QTextStream(&m_keywords) << m_displayName;

    loadSettings();
    loadSnippetGroup(m_ui.groupCombo->currentIndex());

    connect(editorAt(Snippet::Cpp), SIGNAL(snippetContentChanged()),
            this, SLOT(updateSnippetContent()));
    connect(editorAt(Snippet::Qml), SIGNAL(snippetContentChanged()),
            this, SLOT(updateSnippetContent()));
    connect(editorAt(Snippet::PlainText), SIGNAL(snippetContentChanged()),
            this, SLOT(updateSnippetContent()));

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

    connect(m_ui.groupCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(loadSnippetGroup(int)));
    connect(m_ui.addButton, SIGNAL(clicked()), this, SLOT(addSnippet()));
    connect(m_ui.removeButton, SIGNAL(clicked()), this, SLOT(removeSnippet()));
    connect(m_ui.snippetsTable->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(previewSnippet(QModelIndex)));
}

void SnippetsSettingsPagePrivate::decorateEditor(SnippetEditor *editor, Snippet::Group group)
{
    const QList<ISnippetEditorDecorator *> &decorators =
        ExtensionSystem::PluginManager::instance()->getObjects<ISnippetEditorDecorator>();
    foreach (ISnippetEditorDecorator *decorator, decorators)
        if (decorator->supports(group))
            decorator->apply(editor);
}

void SnippetsSettingsPagePrivate::apply()
{
    if (settingsChanged())
        writeSettings();

    if (m_snippetsCollectionChanged) {
        SnippetsManager::instance()->snippetsCollection()->synchronize();
        m_snippetsCollectionChanged = false;
    }
}

void SnippetsSettingsPagePrivate::finish()
{
    if (m_snippetsCollectionChanged) {
        SnippetsManager::instance()->snippetsCollection()->reload();
        m_snippetsCollectionChanged = false;
    }
}

void SnippetsSettingsPagePrivate::loadSettings()
{
    if (QSettings *s = Core::ICore::instance()->settings()) {
        m_settings.fromSettings(m_settingsPrefix, s);
        m_ui.groupCombo->setCurrentIndex(toSnippetGroup(m_settings.lastUsedSnippetGroup()));
    }
}

void SnippetsSettingsPagePrivate::writeSettings()
{
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
    m_ui.snippetsEditorStack->setCurrentIndex(index);
    currentEditor()->clear();
    m_model->load(Snippet::Group(index));
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
}

void SnippetsSettingsPagePrivate::previewSnippet(const QModelIndex &modelIndex)
{
    currentEditor()->setPlainText(m_model->snippetAt(modelIndex).content());
}

void SnippetsSettingsPagePrivate::updateSnippetContent()
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
