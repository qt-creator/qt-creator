/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
