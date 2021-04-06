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

#include "annotationeditordialog.h"

namespace QmlDesigner {

namespace Ui {
class GlobalAnnotationEditorDialog;
}

class GlobalAnnotationEditorDialog : public BasicAnnotationEditorDialog
{
    Q_OBJECT
public:
    enum ViewMode {
        TableView,
        TabsView
    };

    explicit GlobalAnnotationEditorDialog(
        QWidget *parent = nullptr, GlobalAnnotationStatus status = GlobalAnnotationStatus::NoStatus);
    ~GlobalAnnotationEditorDialog();

    ViewMode viewMode() const;

    void setStatus(GlobalAnnotationStatus status);
    GlobalAnnotationStatus globalStatus() const;

public slots:
    void showStatusContainer(bool show);
    void switchToTabView();
    void switchToTableView();

private slots:
    void acceptedClicked() override;

private:

    void fillFields() override;
    void updateAnnotation();
    void addComment(const Comment &comment);
    void removeComment(int index);

    void setStatusVisibility(bool hasStatus);

private:
    const QString globalEditorTitle = {tr("Global Annotation Editor")};

    Ui::GlobalAnnotationEditorDialog *ui;

    GlobalAnnotationStatus m_globalStatus;
    bool m_statusIsActive;
};

} //namespace QmlDesigner
