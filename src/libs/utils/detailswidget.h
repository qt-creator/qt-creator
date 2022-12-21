// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QWidget>

namespace Utils {

class DetailsWidgetPrivate;
class FadingPanel;

class QTCREATOR_UTILS_EXPORT DetailsWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString summaryText READ summaryText WRITE setSummaryText DESIGNABLE true)
    Q_PROPERTY(QString additionalSummaryText READ additionalSummaryText WRITE setAdditionalSummaryText DESIGNABLE true)
    Q_PROPERTY(bool useCheckBox READ useCheckBox WRITE setUseCheckBox DESIGNABLE true)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked DESIGNABLE true)
    Q_PROPERTY(State state READ state WRITE setState)

public:
    enum State {
        Expanded,
        Collapsed,
        NoSummary,
        OnlySummary
    };
    Q_ENUM(State)

    explicit DetailsWidget(QWidget *parent = nullptr);
    ~DetailsWidget() override;

    void setSummaryText(const QString &text);
    QString summaryText() const;

    void setAdditionalSummaryText(const QString &text);
    QString additionalSummaryText() const;

    void setState(State state);
    State state() const;

    void setWidget(QWidget *widget);
    QWidget *widget() const;
    QWidget *takeWidget();

    void setToolWidget(FadingPanel *widget);
    QWidget *toolWidget() const;

    void setSummaryFontBold(bool b);

    bool isChecked() const;
    void setChecked(bool b);

    bool useCheckBox();
    void setUseCheckBox(bool b);
    void setCheckable(bool b);
    void setExpandable(bool b);
    void setIcon(const QIcon &icon);

signals:
    void checked(bool);
    void linkActivated(const QString &link);
    void expanded(bool);

private:
    void setExpanded(bool);

protected:
    void paintEvent(QPaintEvent *paintEvent) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    DetailsWidgetPrivate *d;
};

} // namespace Utils
