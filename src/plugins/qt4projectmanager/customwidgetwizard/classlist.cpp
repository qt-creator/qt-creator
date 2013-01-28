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

#include "classlist.h"

#include <utils/qtcassert.h>

#include <QKeyEvent>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QLabel>
#include <QToolButton>

#include <QDebug>
#include <QRegExp>

namespace Qt4ProjectManager {
namespace Internal {

// ClassModel: Validates the class name in setData() and
// refuses placeholders and invalid characters.
class ClassModel : public QStandardItemModel {
public:
    explicit ClassModel(QObject *parent = 0);
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    void appendPlaceHolder() { appendClass(m_newClassPlaceHolder); }

    QModelIndex placeHolderIndex() const;
    QString newClassPlaceHolder() const { return m_newClassPlaceHolder; }

private:
    void appendClass(const QString &);

    QRegExp m_validator;
    const QString m_newClassPlaceHolder;
};

ClassModel::ClassModel(QObject *parent) :
    QStandardItemModel(0, 1, parent),
    m_validator(QLatin1String("^[a-zA-Z][a-zA-Z0-9_]*$")),
    m_newClassPlaceHolder(ClassList::tr("<New class>"))
{
    QTC_ASSERT(m_validator.isValid(), return);
    appendPlaceHolder();
}

void ClassModel::appendClass(const QString &c)
{
    QStandardItem *item = new QStandardItem(c);
    item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsEditable);
    appendRow(item);
}

bool ClassModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole && !m_validator.exactMatch(value.toString()))
        return false;
    return QStandardItemModel::setData(index, value, role);
}

QModelIndex ClassModel::placeHolderIndex() const
{
    return index(rowCount() - 1, 0);
}

// --------------- ClassList
ClassList::ClassList(QWidget *parent) :
    QListView(parent),
    m_model(new ClassModel)
{
    setModel(m_model);
    connect(itemDelegate(), SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)), SLOT(classEdited()));
    connect(selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotCurrentRowChanged(QModelIndex,QModelIndex)));
}

void ClassList::startEditingNewClassItem()
{
    // Start editing the 'new class' item.
    setFocus();

    const QModelIndex index = m_model->placeHolderIndex();
    setCurrentIndex(index);
    edit(index);
}

QString ClassList::className(int row) const
{
    return m_model->item(row, 0)->text();
}

void ClassList::classEdited()
{
    const QModelIndex index = currentIndex();
    QTC_ASSERT(index.isValid(), return);

    const QString name = className(index.row());
    if (index == m_model->placeHolderIndex()) {
        // Real name class entered.
        if (name != m_model->newClassPlaceHolder()) {
            emit classAdded(name);
            m_model->appendPlaceHolder();
        }
    } else {
        emit classRenamed(index.row(), name);
    }
}

void ClassList::removeCurrentClass()
{
    const QModelIndex index = currentIndex();
    if (!index.isValid() || index == m_model->placeHolderIndex())
        return;
    if (QMessageBox::question(this,
                              tr("Confirm Delete"),
                              tr("Delete class %1 from list?").arg(className(index.row())),
                              QMessageBox::Ok|QMessageBox::Cancel) != QMessageBox::Ok)
        return;
    // Delete row and set current on same item.
    m_model->removeRows(index.row(), 1);
    emit classDeleted(index.row());
    setCurrentIndex(m_model->indexFromItem(m_model->item(index.row(), 0)));
}

void ClassList::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Delete:
        removeCurrentClass();
        break;
    case Qt::Key_Insert:
        startEditingNewClassItem();
        break;
    default:
        QListView::keyPressEvent(event);
        break;
    }
}

void ClassList::slotCurrentRowChanged(const QModelIndex &current, const QModelIndex &)
{
    emit currentRowChanged(current.row());
}

} // namespace Internal
} // namespace Qt4ProjectManager
