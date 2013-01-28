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

#ifndef OPENEDITORSMODEL_H
#define OPENEDITORSMODEL_H

#include "../core_global.h"
#include "../id.h"

#include <QAbstractItemModel>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Core {

struct OpenEditorsModelPrivate;
class IEditor;
class IDocument;

class CORE_EXPORT OpenEditorsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit OpenEditorsModel(QObject *parent);
    virtual ~OpenEditorsModel();

    QIcon lockedIcon() const;
    QIcon unlockedIcon() const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex &/*index*/) const { return QModelIndex(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;

    void addEditor(IEditor *editor, bool isDuplicate = false);
    void addRestoredEditor(const QString &fileName, const QString &displayName, const Id &id);
    QModelIndex firstRestoredEditor() const;

    struct CORE_EXPORT Entry {
        Entry();
        IEditor *editor;
        QString fileName() const;
        QString displayName() const;
        Id id() const;
        QString m_fileName;
        QString m_displayName;
        Id m_id;
    };
    QList<Entry> entries() const;

    IEditor *editorAt(int row) const;

    void removeEditor(IEditor *editor);
    void removeEditor(const QModelIndex &index);
    void removeEditor(const QString &fileName);

    void removeAllRestoredEditors();
    void emitDataChanged(IEditor *editor);

    QList<IEditor *> editors() const;
    QList<Entry> restoredEditors() const;
    bool isDuplicate(IEditor *editor) const;
    QList<IEditor *> duplicatesFor(IEditor *editor) const;
    IEditor *originalForDuplicate(IEditor *duplicate) const;
    void makeOriginal(IEditor *duplicate);
    QModelIndex indexOf(IEditor *editor) const;

    QString displayNameForDocument(IDocument *document) const;

private slots:
    void itemChanged();

private:
    void addEntry(const Entry &entry);
    int findEditor(IEditor *editor) const;
    int findFileName(const QString &filename) const;
    void removeEditor(int idx);

    OpenEditorsModelPrivate *d;
};

} // namespace Core

#endif // OPENEDITORSMODEL_H
