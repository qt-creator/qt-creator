/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef BASICLAYOUTS_H
#define BASICLAYOUTS_H

#include <qlayoutobject.h>
#include <QHBoxLayout>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class QBoxLayoutObject : public QLayoutObject
{
    Q_OBJECT

    Q_PROPERTY(QmlList<QWidget *> *children READ children)

    Q_PROPERTY(int topMargin READ topMargin WRITE setTopMargin)
    Q_PROPERTY(int bottomMargin READ bottomMargin WRITE setBottomMargin)
    Q_PROPERTY(int leftMargin READ leftMargin WRITE setLeftMargin)
    Q_PROPERTY(int rightMargin READ rightMargin WRITE setRightMargin)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing)

    Q_CLASSINFO("DefaultProperty", "children")
public:
    QBoxLayoutObject(QObject *parent=0);
    explicit QBoxLayoutObject(QBoxLayout *, QObject *parent=0);
    virtual QLayout *layout() const;

    QmlList<QWidget *> *children() { return &_widgets; }

private:
    friend class WidgetList;
    void addWidget(QWidget *);
    void clearWidget();

    //XXX need to provide real implementations once QBoxLayoutObject is finished
    class WidgetList : public QmlList<QWidget *>
    {
    public:
        WidgetList(QBoxLayoutObject *o)
            : obj(o) {}

        virtual void append(QWidget *w)  { obj->addWidget(w); }
        virtual void clear() { obj->clearWidget(); }
        virtual int count() const { return 0; }
        virtual void removeAt(int) {}
        virtual QWidget *at(int) const { return 0; }
        virtual void insert(int, QWidget *) {}

    private:
        QBoxLayoutObject *obj;
    };

    void getMargins()
    {
        _layout->getContentsMargins(&mLeft, &mTop, &mRight, &mBottom);
    }

    void setMargins()
    {
        _layout->setContentsMargins(mLeft, mTop, mRight, mBottom);
    }

    int topMargin()
    {
        getMargins();
        return mTop;
    }

    void setTopMargin(int margin)
    {
        getMargins();
        mTop = margin;
        setMargins();
    }

    int bottomMargin()
    {
        getMargins();
        return mBottom;
    }

    void setBottomMargin(int margin)
    {
        getMargins();
        mBottom = margin;
        setMargins();
    }

    int leftMargin()
    {
        getMargins();
        return mLeft;
    }

    void setLeftMargin(int margin)
    {
        getMargins();
        mLeft = margin;
        setMargins();
    }

    int rightMargin()
    {
        getMargins();
        return mRight;
    }

    void setRightMargin(int margin)
    {
        getMargins();
        mRight = margin;
        setMargins();
    }

    int spacing() const
    {
        return _layout->spacing();
    }

    void setSpacing(int spacing)
    {
        _layout->setSpacing(spacing);
    }

    WidgetList _widgets;
    QBoxLayout *_layout;

    int mTop, mLeft, mBottom, mRight;

};

class QHBoxLayoutObject : public QBoxLayoutObject
{
Q_OBJECT
public:
    QHBoxLayoutObject(QObject *parent=0);
};

class QVBoxLayoutObject : public QBoxLayoutObject
{
Q_OBJECT
public:
    QVBoxLayoutObject(QObject *parent=0);
};


QML_DECLARE_TYPE(QBoxLayoutObject);
QML_DECLARE_TYPE(QHBoxLayoutObject);
QML_DECLARE_TYPE(QVBoxLayoutObject);

QT_END_NAMESPACE

QT_END_HEADER

#endif // BASICLAYOUTS_H
