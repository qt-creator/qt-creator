// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QStandardItemModel>
#include <QPointer>

namespace QmlDesigner {

class NodeGraphEditorView;

class NodeGraphEditorModel : public QStandardItemModel
{
    Q_OBJECT

public:
    NodeGraphEditorModel(NodeGraphEditorView *nodeGraphEditorView);
    Q_INVOKABLE void openFile(QString filePath);
    Q_INVOKABLE void openFileName(QString filePath);
    Q_INVOKABLE void saveFile(QString fileName);
private:
    QPointer<NodeGraphEditorView> m_editorView;


Q_PROPERTY(bool hasUnsavedChanges MEMBER m_hasUnsavedChanges WRITE setHasUnsavedChanges NOTIFY hasUnsavedChangesChanged)
    Q_PROPERTY(QString currentFileName READ currentFileName WRITE setCurrentFileName NOTIFY currentFileNameChanged)
    Q_PROPERTY(QString graphData READ graphData WRITE setGraphData NOTIFY graphDataChanged)
 signals:
void graphDataChanged();
void nodeGraphLoaded();
    void hasUnsavedChangesChanged();
    void currentFileNameChanged();
    public:
    void setGraphData(QString val);
    QString graphData();
    bool hasUnsavedChanges() const;
    void setHasUnsavedChanges(bool val);
    QString currentFileName() const;
    void setCurrentFileName(const QString &newCurrentFileName);
private:
    QString saveDirectory();
    QString m_graphData{""};
    QString m_currentFileName{""};
    QString m_saveDirectory{""};
    bool m_hasUnsavedChanges{false};
};

} // namespace QmlDesigner
