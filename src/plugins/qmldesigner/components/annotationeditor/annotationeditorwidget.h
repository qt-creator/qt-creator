// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "annotation.h"

#include <QWidget>


QT_BEGIN_NAMESPACE
class QAbstractButton;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Ui {
class AnnotationEditorWidget;
}
class DefaultAnnotationsModel;

class AnnotationEditorWidget : public QWidget
{
    Q_OBJECT
public:
    enum class ViewMode { TableView, TabsView };

    explicit AnnotationEditorWidget(QWidget *parent,
                                    const QString &targetId = {},
                                    const QString &customId = {});
    ~AnnotationEditorWidget();

    ViewMode viewMode() const;

    const Annotation &annotation() const;
    void setAnnotation(const Annotation &annotation);

    QString targetId() const;
    void setTargetId(const QString &targetId);

    const QString &customId() const;
    void setCustomId(const QString &customId);

    bool isGlobal() const;
    void setGlobal(bool = true);

    DefaultAnnotationsModel *defaultAnnotations() const;
    void loadDefaultAnnotations(const QString &filename);

    GlobalAnnotationStatus globalStatus() const;
    void setStatus(GlobalAnnotationStatus status);

    void updateAnnotation();

public slots:
    void showStatusContainer(bool show);
    void switchToTabView();
    void switchToTableView();

signals:
    void globalChanged();

private:
    void fillFields();

    void addComment(const Comment &comment);
    void removeComment(int index);

    void setStatusVisibility(bool hasStatus);

private:
    std::unique_ptr<DefaultAnnotationsModel> m_defaults;
    std::unique_ptr<Ui::AnnotationEditorWidget> ui;
    GlobalAnnotationStatus m_globalStatus = GlobalAnnotationStatus::NoStatus;
    bool m_statusIsActive = false;
    bool m_isGlobal = false;
    Annotation m_annotation;
    QString m_customId;
};


} //namespace QmlDesigner
