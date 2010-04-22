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

#include "environmenteditmodel.h"

#include <utils/detailswidget.h>


#include <QtGui/QTextDocument>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QToolButton>
#include <QtCore/QDebug>
#include <QtGui/QWidget>
#include <QtGui/QCheckBox>
#include <QtGui/QTableView>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QStackedWidget>

using namespace ProjectExplorer;

EnvironmentModel::EnvironmentModel()
{}

EnvironmentModel::~EnvironmentModel()
{}

QString EnvironmentModel::indexToVariable(const QModelIndex &index) const
{
    return m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row());
}

void EnvironmentModel::updateResultEnvironment()
{
    m_resultEnvironment = m_baseEnvironment;
    m_resultEnvironment.modify(m_items);
    // Add removed variables again and mark them as "<UNSET>" so
    // that the user can actually see those removals:
    foreach (const EnvironmentItem &item, m_items) {
        if (item.unset) {
            m_resultEnvironment.set(item.name, tr("<UNSET>"));
        }
    }
}

void EnvironmentModel::setBaseEnvironment(const ProjectExplorer::Environment &env)
{
    if (m_baseEnvironment == env)
        return;
    beginResetModel();
    m_baseEnvironment = env;
    updateResultEnvironment();
    endResetModel();
}

int EnvironmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_resultEnvironment.size();
}
int EnvironmentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

bool EnvironmentModel::changes(const QString &name) const
{
    return findInChanges(name) >= 0;
}

QVariant EnvironmentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
        if (index.column() == 0) {
            return m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row());
        } else if (index.column() == 1) {
            // Do not return "<UNSET>" when editing a previously unset variable:
            if (role == Qt::EditRole) {
                int pos = findInChanges(indexToVariable(index));
                if (pos >= 0)
                    return m_items.at(pos).value;
            }
            return m_resultEnvironment.value(m_resultEnvironment.constBegin() + index.row());
        }
    }
    if (role == Qt::FontRole) {
        // check whether this environment variable exists in m_items
        if (changes(m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row()))) {
            QFont f;
            f.setBold(true);
            return QVariant(f);
        }
        return QFont();
    }
    return QVariant();
}

Qt::ItemFlags EnvironmentModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

QVariant EnvironmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Variable") : tr("Value");
}

/// *****************
/// Utility functions
/// *****************
int EnvironmentModel::findInChanges(const QString &name) const
{
    for (int i=0; i<m_items.size(); ++i)
        if (m_items.at(i).name == name)
            return i;
    return -1;
}

QModelIndex EnvironmentModel::variableToIndex(const QString &name) const
{
    int row = findInResult(name);
    if (row == -1)
        return QModelIndex();
    return index(row, 0);
}

int EnvironmentModel::findInResult(const QString &name) const
{
    Environment::const_iterator it;
    int i = 0;
    for (it = m_resultEnvironment.constBegin(); it != m_resultEnvironment.constEnd(); ++it, ++i)
        if (m_resultEnvironment.key(it) == name)
            return i;
    return -1;
}

int EnvironmentModel::findInResultInsertPosition(const QString &name) const
{
    Environment::const_iterator it;
    int i = 0;
    for (it = m_resultEnvironment.constBegin(); it != m_resultEnvironment.constEnd(); ++it, ++i)
        if (m_resultEnvironment.key(it) > name)
            return i;
    return m_resultEnvironment.size();
}

bool EnvironmentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    // ignore changes to already set values:
    if (data(index, role) == value)
        return true;

    const QString oldName = data(this->index(index.row(), 0, QModelIndex())).toString();
    const QString oldValue = data(this->index(index.row(), 1, QModelIndex())).toString();
    int changesPos = findInChanges(oldName);

    if (index.column() == 0) {
        //fail if a variable with the same name already exists
#if defined(Q_OS_WIN)
        const QString &newName = value.toString().toUpper();
#else
        const QString &newName = value.toString();
#endif
        // Does the new name exist already?
        if (m_resultEnvironment.hasKey(newName))
            return false;

        EnvironmentItem newVariable(newName, oldValue);

        if (changesPos != -1)
            resetVariable(oldName); // restore the original base variable again

        QModelIndex newIndex = addVariable(newVariable); // add the new variable
        emit focusIndex(newIndex.sibling(newIndex.row(), 1)); // hint to focus on the value
        return true;
    } else if (index.column() == 1) {
        // We are changing an existing value:
        const QString stringValue = value.toString();
        if (changesPos != -1) {
            // We have already changed this value
            if (stringValue == m_baseEnvironment.value(oldName)) {
                // ... and now went back to the base value
                m_items.removeAt(changesPos);
            } else {
                // ... and changed it again
                m_items[changesPos].value = stringValue;
                m_items[changesPos].unset = false;
            }
        } else {
            // Add a new change item:
            m_items.append(EnvironmentItem(oldName, stringValue));
        }
        updateResultEnvironment();
        emit dataChanged(index, index);
        emit userChangesChanged();
        return true;
    }
    return false;
}

QModelIndex EnvironmentModel::addVariable()
{
    return addVariable(EnvironmentItem(tr("<VARIABLE>",
                                          "Name when inserting a new variable"),
                                       tr("<VALUE>",
                                          "Value when inserting a new variable")));
}

QModelIndex EnvironmentModel::addVariable(const EnvironmentItem &item)
{
    int insertPos = findInResultInsertPosition(item.name);

    // Return existing index if the name is already in the result set:
    if (insertPos < m_resultEnvironment.size()
        && m_resultEnvironment.key(m_resultEnvironment.constBegin() + insertPos) == item.name) {
        return index(insertPos, 0, QModelIndex());
    }

    int changePos = findInChanges(item.name);
    if (m_baseEnvironment.hasKey(item.name)) {
        // We previously unset this!
        Q_ASSERT(changePos >= 0);
        // Do not insert a line here as we listed the variable as <UNSET> before!
        Q_ASSERT(m_items.at(changePos).name == item.name);
        Q_ASSERT(m_items.at(changePos).unset);
        Q_ASSERT(m_items.at(changePos).value.isEmpty());
        m_items[changePos] = item;
        emit dataChanged(index(insertPos, 0, QModelIndex()), index(insertPos, 1, QModelIndex()));
    } else {
        // We add something that is not in the base environment
        // Insert a new line!
        beginInsertRows(QModelIndex(), insertPos, insertPos);
        Q_ASSERT(changePos < 0);
        m_items.append(item);
        updateResultEnvironment();
        endInsertRows();
    }
    emit userChangesChanged();
    return index(insertPos, 0, QModelIndex());
}

void EnvironmentModel::resetVariable(const QString &name)
{
    int rowInChanges = findInChanges(name);
    if (rowInChanges < 0)
        return;

    int rowInResult = findInResult(name);
    if (rowInResult < 0)
        return;

    if (m_baseEnvironment.hasKey(name)) {
        m_items.removeAt(rowInChanges);
        updateResultEnvironment();
        emit dataChanged(index(rowInResult, 0, QModelIndex()), index(rowInResult, 1, QModelIndex()));
        emit userChangesChanged();
    } else {
        // Remove the line completely!
        beginRemoveRows(QModelIndex(), rowInResult, rowInResult);
        m_items.removeAt(rowInChanges);
        updateResultEnvironment();
        endRemoveRows();
        emit userChangesChanged();
    }
}

void EnvironmentModel::unsetVariable(const QString &name)
{
    // This does not change the number of rows as we will display a <UNSET>
    // in place of the original variable!
    int row = findInResult(name);
    if (row < 0)
        return;

    // look in m_items for the variable
    int pos = findInChanges(name);
    if (pos != -1) {
        m_items[pos].unset = true;
        m_items[pos].value = QString();
        updateResultEnvironment();
        emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
        emit userChangesChanged();
        return;
    }
    EnvironmentItem item(name, QString());
    item.unset = true;
    m_items.append(item);
    updateResultEnvironment();
    emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
    emit userChangesChanged();
}

bool EnvironmentModel::canUnset(const QString &name)
{
    int pos = findInChanges(name);
    if (pos != -1)
        return m_items.at(pos).unset;
    else
        return false;
}

bool EnvironmentModel::canReset(const QString &name)
{
    return m_baseEnvironment.hasKey(name);
}

QList<EnvironmentItem> EnvironmentModel::userChanges() const
{
    return m_items;
}

void EnvironmentModel::setUserChanges(QList<EnvironmentItem> list)
{
    // We assume nobody is reordering the items here.
    if (list == m_items)
        return;
    beginResetModel();
    m_items = list;
    updateResultEnvironment();
    endResetModel();
}

////
// EnvironmentWidget::EnvironmentWidget
////

EnvironmentWidget::EnvironmentWidget(QWidget *parent, QWidget *additionalDetailsWidget)
    : QWidget(parent)
{
    m_model = new EnvironmentModel();
    connect(m_model, SIGNAL(userChangesChanged()),
            this, SIGNAL(userChangesChanged()));
    connect(m_model, SIGNAL(modelReset()),
            this, SLOT(invalidateCurrentIndex()));

    connect(m_model, SIGNAL(focusIndex(QModelIndex)),
            this, SLOT(focusIndex(QModelIndex)));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    m_detailsContainer = new Utils::DetailsWidget(this);

    QWidget *details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(details);
    details->setVisible(false);

    QVBoxLayout *vbox2 = new QVBoxLayout(details);
    vbox2->setMargin(0);

    if (additionalDetailsWidget)
        vbox2->addWidget(additionalDetailsWidget);

    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->setMargin(0);
    m_environmentView = new QTableView(this);
    m_environmentView->setModel(m_model);
    m_environmentView->setMinimumHeight(400);
    m_environmentView->setGridStyle(Qt::NoPen);
    m_environmentView->horizontalHeader()->setStretchLastSection(true);
    m_environmentView->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_environmentView->horizontalHeader()->setHighlightSections(false);
    m_environmentView->verticalHeader()->hide();
    QFontMetrics fm(font());
    m_environmentView->verticalHeader()->setDefaultSectionSize(qMax(static_cast<int>(fm.height() * 1.2), fm.height() + 4));
    m_environmentView->setSelectionMode(QAbstractItemView::SingleSelection);
    horizontalLayout->addWidget(m_environmentView);

    QVBoxLayout *buttonLayout = new QVBoxLayout();

    m_editButton = new QPushButton(this);
    m_editButton->setText(tr("&Edit"));
    buttonLayout->addWidget(m_editButton);

    m_addButton = new QPushButton(this);
    m_addButton->setText(tr("&Add"));
    buttonLayout->addWidget(m_addButton);

    m_resetButton = new QPushButton(this);
    m_resetButton->setEnabled(false);
    m_resetButton->setText(tr("&Reset"));
    buttonLayout->addWidget(m_resetButton);

    m_unsetButton = new QPushButton(this);
    m_unsetButton->setEnabled(false);
    m_unsetButton->setText(tr("&Unset"));
    buttonLayout->addWidget(m_unsetButton);

    QSpacerItem *verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    buttonLayout->addItem(verticalSpacer);
    horizontalLayout->addLayout(buttonLayout);
    vbox2->addLayout(horizontalLayout);

    vbox->addWidget(m_detailsContainer);

    connect(m_model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            this, SLOT(updateButtons()));

    connect(m_editButton, SIGNAL(clicked(bool)),
            this, SLOT(editEnvironmentButtonClicked()));
    connect(m_addButton, SIGNAL(clicked(bool)),
            this, SLOT(addEnvironmentButtonClicked()));
    connect(m_resetButton, SIGNAL(clicked(bool)),
            this, SLOT(removeEnvironmentButtonClicked()));
    connect(m_unsetButton, SIGNAL(clicked(bool)),
            this, SLOT(unsetEnvironmentButtonClicked()));
    connect(m_environmentView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(environmentCurrentIndexChanged(QModelIndex)));

    connect(m_model, SIGNAL(userChangesChanged()), this, SLOT(updateSummaryText()));
}

EnvironmentWidget::~EnvironmentWidget()
{
    delete m_model;
    m_model = 0;
}

void EnvironmentWidget::focusIndex(const QModelIndex &index)
{
    m_environmentView->setCurrentIndex(index);
    m_environmentView->setFocus();
}

void EnvironmentWidget::setBaseEnvironment(const ProjectExplorer::Environment &env)
{
    m_model->setBaseEnvironment(env);
}

void EnvironmentWidget::setBaseEnvironmentText(const QString &text)
{
    m_baseEnvironmentText = text;
    updateSummaryText();
}

QList<EnvironmentItem> EnvironmentWidget::userChanges() const
{
    return m_model->userChanges();
}

void EnvironmentWidget::setUserChanges(QList<EnvironmentItem> list)
{
    m_model->setUserChanges(list);
    updateSummaryText();
}

void EnvironmentWidget::updateSummaryText()
{
    QString text;
    const QList<EnvironmentItem> &list = m_model->userChanges();
    foreach (const EnvironmentItem &item, list) {
        if (item.name != EnvironmentModel::tr("<VARIABLE>")) {
            text.append("<br>");
            if (item.unset)
                text.append(tr("Unset <b>%1</b>").arg(Qt::escape(item.name)));
            else
                text.append(tr("Set <b>%1</b> to <b>%2</b>").arg(Qt::escape(item.name), Qt::escape(item.value)));
        }
    }

    if (text.isEmpty())
        text.prepend(tr("Using <b>%1</b>").arg(m_baseEnvironmentText));
    else
        text.prepend(tr("Using <b>%1</b> and").arg(m_baseEnvironmentText));

    m_detailsContainer->setSummaryText(text);
}

void EnvironmentWidget::updateButtons()
{
    environmentCurrentIndexChanged(m_environmentView->currentIndex());
}

void EnvironmentWidget::editEnvironmentButtonClicked()
{
    m_environmentView->edit(m_environmentView->currentIndex());
}

void EnvironmentWidget::addEnvironmentButtonClicked()
{
    QModelIndex index = m_model->addVariable();
    m_environmentView->setCurrentIndex(index);
    m_environmentView->edit(index);
}

void EnvironmentWidget::removeEnvironmentButtonClicked()
{
    const QString &name = m_model->indexToVariable(m_environmentView->currentIndex());
    m_model->resetVariable(name);
}

// unset in Merged Environment Mode means, unset if it comes from the base environment
// or remove when it is just a change we added
void EnvironmentWidget::unsetEnvironmentButtonClicked()
{
    const QString &name = m_model->indexToVariable(m_environmentView->currentIndex());
    if (!m_model->canReset(name))
        m_model->resetVariable(name);
    else
        m_model->unsetVariable(name);
}

void EnvironmentWidget::environmentCurrentIndexChanged(const QModelIndex &current)
{
    if (current.isValid()) {
        m_editButton->setEnabled(true);
        const QString &name = m_model->indexToVariable(current);
        bool modified = m_model->canReset(name) && m_model->changes(name);
        bool unset = m_model->canUnset(name);
        m_resetButton->setEnabled(modified || unset);
        m_unsetButton->setEnabled(!unset);
    } else {
        m_editButton->setEnabled(false);
        m_resetButton->setEnabled(false);
        m_unsetButton->setEnabled(false);
    }
}

void EnvironmentWidget::invalidateCurrentIndex()
{
    environmentCurrentIndexChanged(QModelIndex());
}
