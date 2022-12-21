// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QWidgetAction>

#include <theme.h>

namespace QmlDesigner {

class LineEditAction : public QWidgetAction
{
    Q_OBJECT
public:
    explicit LineEditAction(const QString &placeHolderText, QObject *parent = nullptr);

    void setLineEditText(const QString &text);
    void clearLineEditText();

protected:
    QWidget *createWidget(QWidget *parent) override;

signals:
    void textChanged(const QString &text);
    void lineEditTextChange(const QString &text);
    void lineEditTextClear();

private:
    QString m_placeHolderText;
};

} // namespace QmlDesigner
