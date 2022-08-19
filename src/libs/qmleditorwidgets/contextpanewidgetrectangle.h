// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleditorwidgets_global.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class ContextPaneWidgetRectangle; }
QT_END_NAMESPACE

namespace QmlJS { class PropertyReader; }

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT ContextPaneWidgetRectangle : public QWidget
{
    Q_OBJECT

public:
    explicit ContextPaneWidgetRectangle(QWidget *parent = nullptr);
    ~ContextPaneWidgetRectangle();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void enabableGradientEditing(bool);

    void onBorderColorButtonToggled(bool);
    void onColorButtonToggled(bool);
    void onColorDialogApplied(const QColor &color);
    void onColorDialogCancled();
    void onGradientClicked();
    void onColorNoneClicked();
    void onColorSolidClicked();
    void onBorderNoneClicked();
    void onBorderSolidClicked();
    void onGradientLineDoubleClicked(const QPoint &);
    void onUpdateGradient();

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool removeFirst);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void setColor();
    bool isGradientEditingEnabled() const
    { return m_enableGradientEditing; }
    Ui::ContextPaneWidgetRectangle *ui;
    bool m_hasBorder = false;
    bool m_hasGradient = false;
    bool m_none = false;
    bool m_gradientLineDoubleClicked = false;
    int m_gradientTimer = -1;
    bool m_enableGradientEditing = true;
};

} //QmlDesigner
