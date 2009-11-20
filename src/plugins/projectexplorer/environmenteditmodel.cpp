/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtGui/QVBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QToolButton>
#include <QtCore/QDebug>
#include <QtGui/QWidget>
#include <QtGui/QCheckBox>
#include <QtGui/QTreeView>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QStackedWidget>

using namespace ProjectExplorer;

EnvironmentModel::EnvironmentModel()
    : m_mergedEnvironments(false)
{}

EnvironmentModel::~EnvironmentModel()
{}

QString EnvironmentModel::indexToVariable(const QModelIndex &index) const
{
    if (m_mergedEnvironments)
        return m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row());
    else
        return m_items.at(index.row()).name;
}

void EnvironmentModel::updateResultEnvironment()
{
    m_resultEnvironment = m_baseEnvironment;
    m_resultEnvironment.modify(m_items);
    foreach (const EnvironmentItem &item, m_items) {
        if (item.unset) {
            m_resultEnvironment.set(item.name, tr("<UNSET>"));
        }
    }
}

void EnvironmentModel::setBaseEnvironment(const ProjectExplorer::Environment &env)
{
    m_baseEnvironment = env;
    updateResultEnvironment();
    reset();
}

void EnvironmentModel::setMergedEnvironments(bool b)
{
    if (m_mergedEnvironments == b)
        return;
    m_mergedEnvironments = b;
    if (b)
        updateResultEnvironment();
    reset();
}

bool EnvironmentModel::mergedEnvironments()
{
    return m_mergedEnvironments;
}

int EnvironmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_mergedEnvironments ? m_resultEnvironment.size() : m_items.count();
}
int EnvironmentModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

bool EnvironmentModel::changes(const QString &name) const
{
    foreach (const EnvironmentItem& item, m_items)
        if (item.name == name)
            return true;
    return false;
}

QVariant EnvironmentModel::data(const QModelIndex &index, int role) const
{
    if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.isValid()) {
        if ((m_mergedEnvironments && index.row() >= m_resultEnvironment.size()) ||
           (!m_mergedEnvironments && index.row() >= m_items.count())) {
            return QVariant();
        }

        if (index.column() == 0) {
            if (m_mergedEnvironments) {
                return m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row());
            } else {
                return m_items.at(index.row()).name;
            }
        } else if (index.column() == 1) {
            if (m_mergedEnvironments) {
                if (role == Qt::EditRole) {
                    int pos = findInChanges(indexToVariable(index));
                    if (pos != -1)
                        return m_items.at(pos).value;
                }
                return m_resultEnvironment.value(m_resultEnvironment.constBegin() + index.row());
            } else {
                if (m_items.at(index.row()).unset)
                    return tr("<UNSET>");
                else
                    return m_items.at(index.row()).value;
            }
        }
    }
    if (role == Qt::FontRole) {
        if (m_mergedEnvironments) {
            // check wheter this environment variable exists in m_items
            if (changes(m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row()))) {
                QFont f;
                f.setBold(true);
                return QVariant(f);
            }
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

bool EnvironmentModel::hasChildren(const QModelIndex &index) const
{
    if (!index.isValid())
        return true;
    else
        return false;
}

QVariant EnvironmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Variable") : tr("Value");
}

QModelIndex EnvironmentModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, 0);
    return QModelIndex();
}

QModelIndex EnvironmentModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
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

int EnvironmentModel::findInChangesInsertPosition(const QString &name) const
{
    for (int i=0; i<m_items.size(); ++i)
        if (m_items.at(i).name > name)
            return i;
    return m_items.size();
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
    if (role == Qt::EditRole && index.isValid()) {
        // ignore changes to already set values:
        if (data(index, role) == value)
            return true;

        if (index.column() == 0) {
            //fail if a variable with the same name already exists
#ifdef Q_OS_WIN
            const QString &newName = value.toString().toUpper();
#else
            const QString &newName = value.toString();
#endif

            if (findInChanges(newName) != -1)
                return false;

            EnvironmentItem old("", "");
            if (m_mergedEnvironments) {
                int pos = findInChanges(indexToVariable(index));
                if (pos != -1) {
                    old = m_items.at(pos);
                } else {
                    old.name = m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row());
                    old.value = m_resultEnvironment.value(m_resultEnvironment.constBegin() + index.row());
                    old.unset = false;
                }
            } else {
                old = m_items.at(index.row());
            }

            if (changes(old.name))
                removeVariable(old.name);
            old.name = newName;
            addVariable(old);
            return true;
        } else if (index.column() == 1) {
            if (m_mergedEnvironments) {
                const QString &name = indexToVariable(index);
                int pos = findInChanges(name);
                if (pos != -1) {
                    m_items[pos].value = value.toString();
                    m_items[pos].unset = false;
                    updateResultEnvironment();
                    emit dataChanged(index, index);
                    emit userChangesUpdated();
                    return true;
                }
                // not found in m_items, so add it as a new variable
                addVariable(EnvironmentItem(name, value.toString()));
                return true;
            } else {
                m_items[index.row()].value = value.toString();
                m_items[index.row()].unset = false;
                emit dataChanged(index, index);
                emit userChangesUpdated();
                return true;
            }
        }
    }
    return false;
}

QModelIndex EnvironmentModel::addVariable()
{
    const QString name = tr("<VARIABLE>");
    if (m_mergedEnvironments) {
        int i = findInResult(name);
        if (i != -1)
            return index(i, 0, QModelIndex());
    } else {
        int i = findInChanges(name);
        if (i != -1)
            return index(i, 0, QModelIndex());
    }
    // Don't exist, really add them
    return addVariable(EnvironmentItem(name, tr("<VALUE>")));
}

QModelIndex EnvironmentModel::addVariable(const EnvironmentItem &item)
{
    if (m_mergedEnvironments) {
        bool existsInBaseEnvironment = (m_baseEnvironment.find(item.name) != m_baseEnvironment.constEnd());
        int rowInResult;
        if (existsInBaseEnvironment)
            rowInResult = findInResult(item.name);
        else
            rowInResult = findInResultInsertPosition(item.name);
        int rowInChanges = findInChangesInsertPosition(item.name);

        //qDebug() << "addVariable " << item.name << existsInBaseEnvironment << rowInResult << rowInChanges;

        if (existsInBaseEnvironment) {
            m_items.insert(rowInChanges, item);
            updateResultEnvironment();
            emit dataChanged(index(rowInResult, 0, QModelIndex()), index(rowInResult, 1, QModelIndex()));
            emit userChangesUpdated();
            return index(rowInResult, 0, QModelIndex());
        } else {
            beginInsertRows(QModelIndex(), rowInResult, rowInResult);
            m_items.insert(rowInChanges, item);
            updateResultEnvironment();
            endInsertRows();
            emit userChangesUpdated();
            return index(rowInResult, 0, QModelIndex());
        }
    } else {
        int newPos = findInChangesInsertPosition(item.name);
        beginInsertRows(QModelIndex(), newPos, newPos);
        m_items.insert(newPos, item);
        endInsertRows();
        emit userChangesUpdated();
        return index(newPos, 0, QModelIndex());
    }
}

void EnvironmentModel::removeVariable(const QString &name)
{
    if (m_mergedEnvironments) {
        int rowInResult = findInResult(name);
        int rowInChanges = findInChanges(name);
        bool existsInBaseEnvironment = m_baseEnvironment.find(name) != m_baseEnvironment.constEnd();
        if (existsInBaseEnvironment) {
            m_items.removeAt(rowInChanges);
            updateResultEnvironment();
            emit dataChanged(index(rowInResult, 0, QModelIndex()), index(rowInResult, 1, QModelIndex()));
            emit userChangesUpdated();
        } else {
            beginRemoveRows(QModelIndex(), rowInResult, rowInResult);
            m_items.removeAt(rowInChanges);
            updateResultEnvironment();
            endRemoveRows();
            emit userChangesUpdated();
        }
    } else {
        int removePos = findInChanges(name);
        beginRemoveRows(QModelIndex(), removePos, removePos);
        m_items.removeAt(removePos);
        updateResultEnvironment();
        endRemoveRows();
        emit userChangesUpdated();
    }
}

void EnvironmentModel::unset(const QString &name)
{
    if (m_mergedEnvironments) {
        int row = findInResult(name);
        // look in m_items for the variable
        int pos = findInChanges(name);
        if (pos != -1) {
            m_items[pos].unset = true;
            updateResultEnvironment();
            emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
            emit userChangesUpdated();
            return;
        }
        pos = findInChangesInsertPosition(name);
        m_items.insert(pos, EnvironmentItem(name, ""));
        m_items[pos].unset = true;
        updateResultEnvironment();
        emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
        emit userChangesUpdated();
        return;
    } else {
        int pos = findInChanges(name);
        m_items[pos].unset = true;
        emit dataChanged(index(pos, 1, QModelIndex()), index(pos, 1, QModelIndex()));
        emit userChangesUpdated();
        return;
    }
}

bool EnvironmentModel::isUnset(const QString &name)
{
    int pos = findInChanges(name);
    if (pos != -1)
        return m_items.at(pos).unset;
    else
        return false;
}

bool EnvironmentModel::isInBaseEnvironment(const QString &name)
{
    return m_baseEnvironment.find(name) != m_baseEnvironment.constEnd();
}

QList<EnvironmentItem> EnvironmentModel::userChanges() const
{
    return m_items;
}

void EnvironmentModel::setUserChanges(QList<EnvironmentItem> list)
{
    m_items = list;
    updateResultEnvironment();
    emit reset();
}

////
// EnvironmentWidget::EnvironmentWidget
////

EnvironmentWidget::EnvironmentWidget(QWidget *parent, QWidget *additionalDetailsWidget)
    : QWidget(parent)
{
    m_model = new EnvironmentModel();
    m_model->setMergedEnvironments(true);
    connect(m_model, SIGNAL(userChangesUpdated()),
            this, SIGNAL(userChangesUpdated()));

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
    m_environmentTreeView = new QTreeView(this);
    m_environmentTreeView->setRootIsDecorated(false);
    m_environmentTreeView->setHeaderHidden(false);
    m_environmentTreeView->setModel(m_model);
    m_environmentTreeView->header()->resizeSection(0, 250);
    m_environmentTreeView->setMinimumHeight(400);
    horizontalLayout->addWidget(m_environmentTreeView);

    QVBoxLayout *buttonLayout = new QVBoxLayout();

    m_editButton = new QPushButton(this);
    m_editButton->setText(tr("&Edit"));
    buttonLayout->addWidget(m_editButton);

    m_addButton = new QPushButton(this);
    m_addButton->setText(tr("&Add"));
    buttonLayout->addWidget(m_addButton);

    m_removeButton = new QPushButton(this);
    m_removeButton->setEnabled(false);
    m_removeButton->setText(tr("&Reset"));
    buttonLayout->addWidget(m_removeButton);

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
    connect(m_removeButton, SIGNAL(clicked(bool)),
            this, SLOT(removeEnvironmentButtonClicked()));
    connect(m_unsetButton, SIGNAL(clicked(bool)),
            this, SLOT(unsetEnvironmentButtonClicked()));
    connect(m_environmentTreeView->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)),
            this, SLOT(environmentCurrentIndexChanged(QModelIndex, QModelIndex)));

    connect(m_model, SIGNAL(userChangesUpdated()), this, SLOT(updateSummaryText()));
}

EnvironmentWidget::~EnvironmentWidget()
{
    delete m_model;
    m_model = 0;
}

void EnvironmentWidget::setBaseEnvironment(const ProjectExplorer::Environment &env)
{
    m_model->setBaseEnvironment(env);
}

void EnvironmentWidget::setMergedEnvironments(bool b)
{
    m_model->setMergedEnvironments(b);
}

bool EnvironmentWidget::mergedEnvironments()
{
    return m_model->mergedEnvironments();
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
        if (!text.isEmpty())
            text.append("<br>");
	if (item.name != EnvironmentModel::tr("<VARIABLE>")) {
            if (item.unset)
                text.append(tr("Unset <b>%1</b>").arg(item.name));
            else
                text.append(tr("Set <b>%1</b> to <b>%2</b>").arg(item.name, item.value));
        }
    }
    if (text.isEmpty())
        text = tr("Summary: No changes to Environment");
    m_detailsContainer->setSummaryText(text);
}

void EnvironmentWidget::updateButtons()
{
    environmentCurrentIndexChanged(m_environmentTreeView->currentIndex(), QModelIndex());
}

void EnvironmentWidget::editEnvironmentButtonClicked()
{
    m_environmentTreeView->edit(m_environmentTreeView->currentIndex());
}

void EnvironmentWidget::addEnvironmentButtonClicked()
{
    QModelIndex index = m_model->addVariable();
    m_environmentTreeView->setCurrentIndex(index);
    m_environmentTreeView->edit(index);
    updateButtons();
}

void EnvironmentWidget::removeEnvironmentButtonClicked()
{
    const QString &name = m_model->indexToVariable(m_environmentTreeView->currentIndex());
    m_model->removeVariable(name);
    updateButtons();
}

// unset in Merged Environment Mode means, unset if it comes from the base environment
// or remove when it is just a change we added
// unset in changes view, means just unset
void EnvironmentWidget::unsetEnvironmentButtonClicked()
{
    const QString &name = m_model->indexToVariable(m_environmentTreeView->currentIndex());
    if (!m_model->isInBaseEnvironment(name) && m_model->mergedEnvironments())
        m_model->removeVariable(name);
    else
        m_model->unset(name);
    updateButtons();
}

void EnvironmentWidget::environmentCurrentIndexChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    if (current.isValid()) {
        m_editButton->setEnabled(true);
        if (m_model->mergedEnvironments()) {
            const QString &name = m_model->indexToVariable(current);
            bool modified = m_model->isInBaseEnvironment(name) && m_model->changes(name);
            bool unset = m_model->isUnset(name);
            m_removeButton->setEnabled(modified || unset);
            m_unsetButton->setEnabled(!unset);
        } else {
            m_removeButton->setEnabled(true);
            m_unsetButton->setEnabled(!m_model->isUnset(m_model->indexToVariable(current)));
        }
    } else {
        m_editButton->setEnabled(false);
        m_removeButton->setEnabled(false);
        m_unsetButton->setEnabled(false);
    }
}
