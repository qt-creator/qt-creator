// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QWidgetAction>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QComboBox)

namespace QmlDesigner {

class BackgroundAction : public QWidgetAction
{
    enum BackgroundType { CheckboardBackground, WhiteBackground, BlackBackground };

    Q_OBJECT
public:
    enum SpecialColor { ContextImage = Qt::yellow };

    explicit BackgroundAction(QObject *parent);
    void setColor(const QColor &color);

signals:
    void backgroundChanged(const QColor &color);

protected:
    QWidget *createWidget(QWidget *parent) override;

private:
    void emitBackgroundChanged(int index);

private:
    static QList<QColor> colors();
    QPointer<QComboBox> m_comboBox;
};

} // namespace QmlDesigner
