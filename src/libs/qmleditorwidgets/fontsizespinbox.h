/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef FONTSIZESPINBOX_H
#define FONTSIZESPINBOX_H

#include <qmleditorwidgets_global.h>
#include <QAbstractSpinBox>

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT FontSizeSpinBox : public QAbstractSpinBox
{
    Q_OBJECT

     Q_PROPERTY(bool isPixelSize READ isPixelSize WRITE setIsPixelSize NOTIFY formatChanged)
     Q_PROPERTY(bool isPointSize READ isPointSize WRITE setIsPointSize NOTIFY formatChanged)
     Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit FontSizeSpinBox(QWidget *parent = 0);

     bool isPixelSize() { return !m_isPointSize; }
     bool isPointSize() { return m_isPointSize; }

     void stepBy(int steps);

     QValidator::State validate (QString &input, int &pos) const;
     int value() const { return m_value; }

signals:
     void formatChanged();
     void valueChanged(int);

public slots:
     void setIsPointSize(bool b)
     {
         if (isPointSize() == b)
             return;

         m_isPointSize = b;
         setText();
         emit formatChanged();
     }

     void setIsPixelSize(bool b)
     {
         if (isPixelSize() == b)
             return;

         m_isPointSize = !b;
         setText();
         emit formatChanged();
     }


     void clear();
     void setValue (int val);

 protected:
    StepEnabled stepEnabled() const;
private slots:
    void onEditingFinished();
    void setText();

private:
    bool m_isPointSize;
    int m_value;

};

} //QmlDesigner

#endif // FONTSIZESPINBOX_H
