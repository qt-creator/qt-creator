/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QDialog>

#include "annotation.h"

namespace QmlDesigner {

namespace Ui {
class AnnotationEditorDialog;
}
class DefaultAnnotationsModel;

class AnnotationEditorDialog : public QDialog
{
    Q_OBJECT
public:
    enum ViewMode { TableView, TabsView };

    explicit AnnotationEditorDialog(QWidget *parent,
                                    const QString &targetId = {},
                                    const QString &customId = {});
    ~AnnotationEditorDialog();

    ViewMode viewMode() const;

    Annotation const &annotation() const;
    void setAnnotation(const Annotation &annotation);

    QString customId() const;
    void setCustomId(const QString &customId);

    bool isGlobal() const;
    void setGlobal(bool = true);

    void loadDefaultAnnotations(QString const &filename);

    DefaultAnnotationsModel *defaultAnnotations() const;
    void setStatus(GlobalAnnotationStatus status);
    GlobalAnnotationStatus globalStatus() const;

public slots:
    void showStatusContainer(bool show);
    void switchToTabView();
    void switchToTableView();

private slots:
    void acceptedClicked();

signals:
    void acceptedDialog(); //use instead of QDialog::accepted
    void globalChanged();
private:
    void fillFields();
    void updateAnnotation();

    void addComment(const Comment &comment);
    void removeComment(int index);

    void setStatusVisibility(bool hasStatus);

    std::unique_ptr<Ui::AnnotationEditorDialog> ui;
    GlobalAnnotationStatus m_globalStatus = GlobalAnnotationStatus::NoStatus;
    bool m_statusIsActive = false;
    bool m_isGlobal = false;
    Annotation m_annotation;
    QString m_customId;
    std::unique_ptr<DefaultAnnotationsModel> m_defaults;
};


} //namespace QmlDesigner
