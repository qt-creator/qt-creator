// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "propertyeditorview.h"
#include "rewritertransaction.h"

namespace QmlDesigner {

class PropertyEditorTransaction : public QObject
{
    Q_OBJECT
public:
    PropertyEditorTransaction(PropertyEditorView *propertyEditor);

    Q_INVOKABLE void start();
    Q_INVOKABLE void end();

    Q_INVOKABLE bool active() const;

protected:
     void timerEvent(QTimerEvent *event) override;

private:
    PropertyEditorView *m_propertyEditor;
    RewriterTransaction m_rewriterTransaction;
    int m_timerId;
};

} //QmlDesigner
