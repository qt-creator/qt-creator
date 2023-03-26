// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelnode.h"

#include <coreplugin/icore.h>

#include <QWidget>
#include <QDialog>
#include <QListView>


namespace QmlDesigner {

class AnnotationListView;
class AnnotationEditorWidget;

class AnnotationListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnnotationListWidget(ModelNode rootNode, QWidget *parent = nullptr);
    ~AnnotationListWidget() = default;

    void setRootNode(ModelNode rootNode);

    void saveAllChanges();

private:
    void createUI();

    bool validateListSize();

private slots:
    void changeAnnotation(const QModelIndex &index);

private:
    AnnotationListView *m_listView;
    AnnotationEditorWidget *m_editor;

    int m_currentItem = -1;
};

}

