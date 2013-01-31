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

#ifndef FONTSIZESPINBOX_H
#define FONTSIZESPINBOX_H

#include "qmleditorwidgets_global.h"
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
