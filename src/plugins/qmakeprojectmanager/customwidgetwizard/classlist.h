// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QListView>

namespace QmakeProjectManager {
namespace Internal {
class ClassModel;

// Class list for new Custom widget classes. Provides
// editable '<new class>' field and Delete/Insert key handling.
class ClassList : public QListView
{
    Q_OBJECT

public:
    explicit ClassList(QWidget *parent = nullptr);

    QString className(int row) const;

signals:
    void classAdded(const QString &name);
    void classRenamed(int index, const QString &newName);
    void classDeleted(int index);
    void currentRowChanged(int);

public:
    void removeCurrentClass();
    void startEditingNewClassItem();

private:
    void classEdited();
    void slotCurrentRowChanged(const QModelIndex &,const QModelIndex &);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    ClassModel *m_model;
};

} // namespace Internal
} // namespace QmakeProjectManager
