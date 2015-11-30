/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef RESOURCEVIEW_H
#define RESOURCEVIEW_H

#include "resourcefile_p.h"

#include <utils/itemviews.h>

#include <QPoint>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMouseEvent;
class QUndoStack;
QT_END_NAMESPACE

namespace ResourceEditor {
namespace Internal {

class RelativeResourceModel;

class ResourceView : public Utils::TreeView
{
    Q_OBJECT

public:
    enum NodeProperty {
        AliasProperty,
        PrefixProperty,
        LanguageProperty
    };

    explicit ResourceView(RelativeResourceModel *model, QUndoStack *history, QWidget *parent = 0);
    ~ResourceView();

    QString fileName() const;

    bool isPrefix(const QModelIndex &index) const;

    QString currentAlias() const;
    QString currentPrefix() const;
    QString currentLanguage() const;
    QString currentResourcePath() const;

    void setResourceDragEnabled(bool e);
    bool resourceDragEnabled() const;

    void findSamePlacePostDeletionModelIndex(int &row, QModelIndex &parent) const;
    EntryBackup *removeEntry(const QModelIndex &index);
    QStringList existingFilesSubtracted(int prefixIndex, const QStringList &fileNames) const;
    void addFiles(int prefixIndex, const QStringList &fileNames, int cursorFile,
                  int &firstFile, int &lastFile);
    void removeFiles(int prefixIndex, int firstFileIndex, int lastFileIndex);
    QStringList fileNamesToAdd();
    QModelIndex addPrefix();
    QList<QModelIndex> nonExistingFiles();

    void refresh();

public slots:
    void setCurrentAlias(const QString &before, const QString &after);
    void setCurrentPrefix(const QString &before, const QString &after);
    void setCurrentLanguage(const QString &before, const QString &after);
    void advanceMergeId();

protected:
    void keyPressEvent(QKeyEvent *e);

signals:
    void removeItem();
    void itemActivated(const QString &fileName);
    void showContextMenu(const QPoint &globalPos, const QString &fileName);

public:
    QString getCurrentValue(NodeProperty property) const;
    void changeValue(const QModelIndex &nodeIndex, NodeProperty property, const QString &value);

private slots:
    void itemActivated(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);

private:
    void addUndoCommand(const QModelIndex &nodeIndex, NodeProperty property,
                        const QString &before, const QString &after);

    RelativeResourceModel *m_qrcModel;

    QUndoStack *m_history;
    int m_mergeId;
};

} // namespace Internal
} // namespace ResourceEditor

#endif // RESOURCEVIEW_H
