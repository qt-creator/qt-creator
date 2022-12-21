// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtQml>

#include "editorproxy.h"

namespace QmlDesigner {

class AnnotationEditor : public ModelNodeEditorProxy
{
    Q_OBJECT
public:
    explicit AnnotationEditor(QObject *parent = nullptr);
    ~AnnotationEditor() = default;

    QWidget *createWidget() override;
    Q_INVOKABLE void removeFullAnnotation();

    static void registerDeclarativeType();

signals:
    void accepted();
    void canceled();
    void applied();

private slots:
    void acceptedClicked();
    void cancelClicked();
    void appliedClicked();

private:
    void applyChanges();
};

} //namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::AnnotationEditor)
