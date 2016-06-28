/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    Q_ENUMS(State)

public:
    enum State {
        Expanded,
        Collapsed,
        NoSummary,
        OnlySummary
    };

    explicit DetailsWidget(QWidget *parent = 0);
    virtual ~DetailsWidget();

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
    void setIcon(const QIcon &icon);

    static QPixmap createBackground(const QSize &size, int topHeight, QWidget *widget);

signals:
    void checked(bool);
    void linkActivated(const QString &link);
    void expanded(bool);

private:
    void setExpanded(bool);

protected:
    virtual void paintEvent(QPaintEvent *paintEvent);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);

private:
    DetailsWidgetPrivate *d;
};

} // namespace Utils
