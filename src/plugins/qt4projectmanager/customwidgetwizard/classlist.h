/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CLASSLIST_H
#define CLASSLIST_H

#include <QListView>

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
