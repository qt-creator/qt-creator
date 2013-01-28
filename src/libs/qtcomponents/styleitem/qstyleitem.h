/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef STYLEWRAPPER_H
#define STYLEWRAPPER_H

#include <QCheckBox>
#include <QComboBox>
#include <QDeclarativeItem>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHeaderView>
#include <QLineEdit>
#include <QPixmapCache>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QStyle>
#include <QToolButton>
#include <QToolTip>
#include <QTreeView>

class QStyleItem: public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY( bool sunken READ sunken WRITE setSunken NOTIFY sunkenChanged)
    Q_PROPERTY( bool raised READ raised WRITE setRaised NOTIFY raisedChanged)
    Q_PROPERTY( bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY( bool selected READ selected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY( bool focus READ focus WRITE setFocus NOTIFY focusChanged)
    Q_PROPERTY( bool on READ on WRITE setOn NOTIFY onChanged)
    Q_PROPERTY( bool hover READ hover WRITE setHover NOTIFY hoverChanged)
    Q_PROPERTY( bool horizontal READ horizontal WRITE setHorizontal NOTIFY horizontalChanged)

    Q_PROPERTY( QString elementType READ elementType WRITE setElementType NOTIFY elementTypeChanged)
    Q_PROPERTY( QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY( QString activeControl READ activeControl WRITE setActiveControl NOTIFY activeControlChanged)
    Q_PROPERTY( QString info READ info WRITE setInfo NOTIFY infoChanged)
    Q_PROPERTY( QString style READ style NOTIFY styleChanged)
    Q_PROPERTY( QString hint READ hint WRITE setHint NOTIFY hintChanged)
    Q_PROPERTY( QString cursor READ cursor WRITE setCursor NOTIFY cursorChanged)

    // For range controls
    Q_PROPERTY( int minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY( int maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY( int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY( int step READ step WRITE setStep NOTIFY stepChanged)
    Q_PROPERTY( int paintMargins READ paintMargins WRITE setPaintMargins NOTIFY paintMarginsChanged)

    Q_PROPERTY( QString fontFamily READ fontFamily)
    Q_PROPERTY( double fontPointSize READ fontPointSize)
    Q_PROPERTY( int fontHeight READ fontHeight NOTIFY fontHeightChanged)

public:
    enum Type {
        Undefined,
        Button,
        RadioButton,
        CheckBox,
        ComboBox,
        ComboBoxItem,
        Dial,
        ToolBar,
        ToolButton,
        Tab,
        TabFrame,
        Frame,
        FocusFrame,
        SpinBox,
        Slider,
        ScrollBar,
        ProgressBar,
        Edit,
        GroupBox,
        Header,
        Item,
        ItemRow,
        Splitter,
        Menu,
        MenuItem,
        Widget
    };

    QStyleItem(QDeclarativeItem *parent = 0);
    ~QStyleItem();

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    bool sunken() const { return m_sunken; }
    bool raised() const { return m_raised; }
    bool active() const { return m_active; }
    bool selected() const { return m_selected; }
    bool focus() const { return m_focus; }
    bool on() const { return m_on; }
    bool hover() const { return m_hover; }
    bool horizontal() const { return m_horizontal; }

    int minimum() const { return m_minimum; }
    int maximum() const { return m_maximum; }
    int step() const { return m_step; }
    int value() const { return m_value; }
    int paintMargins() const { return m_paintMargins; }

    QString elementType() const { return m_type; }
    QString text() const { return m_text; }
    QString cursor() const { return m_cursor; }
    QString activeControl() const { return m_activeControl; }
    QString info() const { return m_info; }
    QString hint() const { return m_hint; }
    QString style() const;

    void setSunken(bool sunken) { if (m_sunken != sunken) {m_sunken = sunken; emit sunkenChanged();}}
    void setRaised(bool raised) { if (m_raised!= raised) {m_raised = raised; emit raisedChanged();}}
    void setActive(bool active) { if (m_active!= active) {m_active = active; emit activeChanged();}}
    void setSelected(bool selected) { if (m_selected!= selected) {m_selected = selected; emit selectedChanged();}}
    void setFocus(bool focus) { if (m_focus != focus) {m_focus = focus; emit focusChanged();}}
    void setOn(bool on) { if (m_on != on) {m_on = on ; emit onChanged();}}
    void setHover(bool hover) { if (m_hover != hover) {m_hover = hover ; emit hoverChanged();}}
    void setHorizontal(bool horizontal) { if (m_horizontal != horizontal) {m_horizontal = horizontal; emit horizontalChanged();}}
    void setMinimum(int minimum) { if (m_minimum!= minimum) {m_minimum = minimum; emit minimumChanged();}}
    void setMaximum(int maximum) { if (m_maximum != maximum) {m_maximum = maximum; emit maximumChanged();}}
    void setValue(int value) { if (m_value!= value) {m_value = value; emit valueChanged();}}
    void setStep(int step) { if (m_step != step) { m_step = step; emit stepChanged(); }}
    void setPaintMargins(int value) {
    Q_UNUSED(value)
#ifdef Q_WS_WIN //only vista style needs this hack
        if (m_paintMargins!= value) {m_paintMargins = value;}
#endif
    }
    void setCursor(const QString &str);
    void setElementType(const QString &str);
    void setText(const QString &str) { if (m_text != str) {m_text = str; emit textChanged();}}
    void setActiveControl(const QString &str) { if (m_activeControl != str) {m_activeControl = str; emit activeControlChanged();}}
    void setInfo(const QString &str) { if (m_info != str) {m_info = str; emit infoChanged();}}
    void setHint(const QString &str) { if (m_hint != str) {m_hint= str; emit hintChanged();}}

    bool eventFilter(QObject *, QEvent *);
    virtual void initStyleOption ();
    QWidget *widget(){ return m_dummywidget; }

    int fontHeight();
    QString fontFamily();
    double fontPointSize();


public Q_SLOTS:
    int pixelMetric(const QString&);
    QVariant styleHint(const QString&);
    QSize sizeFromContents(int width, int height);
    void updateItem(){update();}
    QString hitTest(int x, int y);
    QRect subControlRect(const QString &subcontrolString);
    void showToolTip(const QString &str);
    int textWidth(const QString &);

Q_SIGNALS:
    void elementTypeChanged();
    void textChanged();
    void sunkenChanged();
    void raisedChanged();
    void activeChanged();
    void selectedChanged();
    void focusChanged();
    void onChanged();
    void hoverChanged();
    void horizontalChanged();
    void minimumChanged();
    void maximumChanged();
    void stepChanged();
    void valueChanged();
    void activeControlChanged();
    void infoChanged();
    void styleChanged();
    void paintMarginsChanged();
    void hintChanged();
    void cursorChanged();
    void fontHeightChanged();

protected:
    QWidget *m_dummywidget;
    QStyleOption *m_styleoption;
    Type m_itemType;

    QString m_type;
    QString m_cursor;
    QString m_text;
    QString m_activeControl;
    QString m_info;
    QString m_hint;

    bool m_sunken;
    bool m_raised;
    bool m_active;
    bool m_selected;
    bool m_focus;
    bool m_hover;
    bool m_on;
    bool m_horizontal;
    bool m_sharedWidget;

    int m_minimum;
    int m_maximum;
    int m_value;
    int m_step;
    int m_paintMargins;
};

#endif //STYLEWRAPPER_H
