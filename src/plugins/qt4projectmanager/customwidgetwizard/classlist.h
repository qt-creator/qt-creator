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

#ifndef CLASSLIST_H
#define CLASSLIST_H

#include <QtGui/QListView>

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace Qt4ProjectManager {
namespace Internal {
class ClassModel;

// Class list for new Custom widget classes. Provides
// editable '<new class>' field and Delete/Insert key handling.
class ClassList : public QListView
{
    Q_OBJECT

public:
    explicit ClassList(QWidget *parent = 0);

    QString className(int row) const;

signals:
    void classAdded(const QString &name);
    void classRenamed(int index, const QString &newName);
    void classDeleted(int index);
    void currentRowChanged(int);

public slots:
    void removeCurrentClass();
    void startEditingNewClassItem();

private slots:
    void classEdited();
    void slotCurrentRowChanged(const QModelIndex &,const QModelIndex &);

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    ClassModel *m_model;
};

} // namespace Internal
} // namespace Qt4ProjectManager
#endif
