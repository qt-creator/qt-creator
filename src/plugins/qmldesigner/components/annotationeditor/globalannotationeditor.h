// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "annotation.h"
#include "editorproxy.h"
#include "modelnode.h"

#include <QObject>
#include <QPointer>
#include <QtQml>


namespace QmlDesigner {

class GlobalAnnotationEditor : public ModelNodeEditorProxy
{
    Q_OBJECT
public:
    explicit GlobalAnnotationEditor(QObject *parent = nullptr);
    ~GlobalAnnotationEditor() = default;

    QWidget *createWidget() override;

    Q_INVOKABLE void removeFullAnnotation();

signals:
    void accepted();
    void canceled();
    void applied();

private slots:
    void acceptedClicked();
    void appliedClicked();
    void cancelClicked();

private:
    void applyChanges();
};

} //namespace QmlDesigner
