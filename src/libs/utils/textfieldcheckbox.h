/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TEXTFIELDCHECKBOX_H
#define TEXTFIELDCHECKBOX_H

#include "utils_global.h"

#include <QCheckBox>

namespace Utils {

// Documentation inside.
class QTCREATOR_UTILS_EXPORT TextFieldCheckBox : public QCheckBox {
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString trueText READ trueText WRITE setTrueText)
    Q_PROPERTY(QString falseText READ falseText WRITE setFalseText)
    Q_OBJECT
public:
    explicit TextFieldCheckBox(const QString &text, QWidget *parent = 0);

    QString text() const;
    void setText(const QString &s);

    void setTrueText(const QString &t)  { m_trueText = t;    }
    QString trueText() const            { return m_trueText; }
    void setFalseText(const QString &t) { m_falseText = t; }
    QString falseText() const              { return m_falseText; }

signals:
    void textChanged(const QString &);

private slots:
    void slotStateChanged(int);

private:
    QString m_trueText;
    QString m_falseText;
};

} // namespace Utils

#endif // TEXTFIELDCHECKBOX_H
