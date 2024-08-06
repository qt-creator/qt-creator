// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QToolButton>
#include <QWidgetAction>

QT_BEGIN_NAMESPACE
class QStyleOptionToolButton;
class QPaintEvent;
class QStyleOptionMenuItem;
QT_END_NAMESPACE

namespace QmlDesigner {

class IndicatorButton : public QToolButton
{
    Q_OBJECT

    Q_PROPERTY(bool indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)
public:
    explicit IndicatorButton(QWidget *parent = nullptr);

    bool indicator() const;
    void setIndicator(bool newIndicator);

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void initMenuStyleOption(QMenu *menu, QStyleOptionMenuItem *option, const QAction *action) const;

signals:
    void indicatorChanged(bool);

private:
    bool m_indicator = false;
    using Super = QToolButton;
};

class IndicatorButtonAction : public QWidgetAction
{
    Q_OBJECT

public:
    explicit IndicatorButtonAction(const QString &description,
                                   const QIcon &icon,
                                   QObject *parent = nullptr);
    virtual ~IndicatorButtonAction();

public slots:
    void setIndicator(bool indicator);

protected:
    virtual QWidget *createWidget(QWidget *parent) override;

private:
    bool m_indicator;

signals:
    void indicatorChanged(bool, QPrivateSignal);
};

} // namespace QmlDesigner
