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

#ifndef OPENEDITORSMODEL_H
#define OPENEDITORSMODEL_H

#include "../core_global.h"

#include <QtCore/QAbstractItemModel>

namespace Core {

class IEditor;
class IFile;

class CORE_EXPORT OpenEditorsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    OpenEditorsModel(QObject *parent) : QAbstractItemModel(parent) {}
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex &/*index*/) const { return QModelIndex(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;

    void addEditor(IEditor *editor, bool isDuplicate = false);
    void addRestoredEditor(const QString &fileName, const QString &displayName, const QString &id);
    QModelIndex firstRestoredEditor() const;

    struct Entry {
        Entry():editor(0){}
        IEditor *editor;
        QString fileName() const;
        QString displayName() const;
        QString id() const;
        QString m_fileName;
        QString m_displayName;
        QString m_id;
    };
    QList<Entry> entries() const { return m_editors; }

    inline IEditor *editorAt(int row) const { return m_editors.at(row).editor; }

    void removeEditor(IEditor *editor);
    void removeEditor(const QModelIndex &index);

    void removeAllRestoredEditors();
    int restoredEditorCount() const;
    void emitDataChanged(IEditor *editor);

    QList<IEditor *> editors() const;
    bool isDuplicate(IEditor *editor) const;
    QList<IEditor *> duplicatesFor(IEditor *editor) const;
    IEditor *originalForDuplicate(IEditor *duplicate) const;
    void makeOriginal(IEditor *duplicate);
    QModelIndex indexOf(IEditor *editor) const;

    QString displayNameForFile(IFile *file) const;

private slots:
    void itemChanged();

private:
    void addEntry(const Entry &entry);
    int findEditor(IEditor *editor) const;
    int findFileName(const QString &filename) const;
    QList<Entry> m_editors;
    QList<IEditor *>m_duplicateEditors;
};

} // namespace Core

#endif // OPENEDITORSMODEL_H
