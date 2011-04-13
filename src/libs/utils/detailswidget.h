/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include "utils_global.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QPixmap;
QT_END_NAMESPACE

namespace Utils {

struct DetailsWidgetPrivate;
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
        NoSummary
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

    void setToolWidget(Utils::FadingPanel *widget);
    QWidget *toolWidget() const;

    void setSummaryFontBold(bool b);

    bool isChecked() const;
    void setChecked(bool b);

    bool useCheckBox();
    void setUseCheckBox(bool b);
    /// Sets an icon, only supported if useCheckBox is true
    void setIcon(const QIcon &icon);

signals:
    void checked(bool);
private slots:
    void setExpanded(bool);

protected:
    virtual void paintEvent(QPaintEvent *paintEvent);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);

private:
    void updateControls();
    QPixmap cacheBackground(const QSize &size, bool expanded);
    void changeHoverState(bool hovered);

    DetailsWidgetPrivate *d;
    QString m_additionalSummaryText;
};

} // namespace Utils

#endif // DETAILSWIDGET_H
