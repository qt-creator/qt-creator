// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "resourcefile_p.h"

#include <utils/itemviews.h>

#include <QPoint>

QT_BEGIN_NAMESPACE
class QUndoStack;
QT_END_NAMESPACE

namespace ResourceEditor::Internal {

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

    explicit ResourceView(RelativeResourceModel *model, QUndoStack *history, QWidget *parent = nullptr);
    ~ResourceView() override;

    Utils::FilePath filePath() const;

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

    void setCurrentAlias(const QString &before, const QString &after);
    void setCurrentPrefix(const QString &before, const QString &after);
    void setCurrentLanguage(const QString &before, const QString &after);
    void advanceMergeId();

    QString getCurrentValue(NodeProperty property) const;
    void changeValue(const QModelIndex &nodeIndex, NodeProperty property, const QString &value);

signals:
    void removeItem();
    void itemActivated(const QString &fileName);
    void contextMenuShown(const QPoint &globalPos, const QString &fileName);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void onItemActivated(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);
    void addUndoCommand(const QModelIndex &nodeIndex, NodeProperty property,
                        const QString &before, const QString &after);

    RelativeResourceModel *m_qrcModel;

    QUndoStack *m_history;
    int m_mergeId;
};

} // ResourceEditor::Internal
