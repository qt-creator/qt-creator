/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

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

    void setToolWidget(Utils::FadingPanel *widget);
    QWidget *toolWidget() const;

    void setSummaryFontBold(bool b);

    bool isChecked() const;
    void setChecked(bool b);

    bool useCheckBox();
    void setUseCheckBox(bool b);
    /// Sets an icon, only supported if useCheckBox is true
    void setIcon(const QIcon &icon);

    static QPixmap createBackground(const QSize &size, int topHeight, QWidget *widget);

signals:
    void checked(bool);
    void linkActivated(const QString &link);
    void expanded(bool);

private slots:
    void setExpanded(bool);

protected:
    virtual void paintEvent(QPaintEvent *paintEvent);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);

private:
    DetailsWidgetPrivate *d;
};

} // namespace Utils

#endif // DETAILSWIDGET_H
