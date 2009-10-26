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

#ifndef BASEVALIDATINGLINEEDIT_H
#define BASEVALIDATINGLINEEDIT_H

#include "utils_global.h"

#include <QtGui/QLineEdit>

namespace Utils {

struct BaseValidatingLineEditPrivate;

/**
 * Base class for validating line edits that performs validation in a virtual
 * validate() function to be implemented in derived classes.
 * When invalid, the text color will turn red and a tooltip will
 * contain the error message. This approach is less intrusive than a
 * QValidator which will prevent the user from entering certain characters.
 *
 * The widget has a concept of an "initialText" which can be something like
 * "<Enter name here>". This results in state 'DisplayingInitialText', which
 * is not valid, but is not marked red.
 */
class QTCREATOR_UTILS_EXPORT BaseValidatingLineEdit : public QLineEdit
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseValidatingLineEdit)
    Q_PROPERTY(QString initialText READ initialText WRITE setInitialText DESIGNABLE true)
    Q_PROPERTY(QColor errorColor READ errorColor WRITE setErrorColor DESIGNABLE true)

public:
    enum State { Invalid, DisplayingInitialText, Valid };

    explicit BaseValidatingLineEdit(QWidget *parent = 0);
    virtual ~BaseValidatingLineEdit();


    State state() const;
    bool isValid() const;
    QString errorMessage() const;

    QString initialText() const;
    void setInitialText(const QString &);

    QColor errorColor() const;
    void setErrorColor(const  QColor &);

    // Trigger an update (after changing settings)
    void triggerChanged();

    static QColor textColor(const QWidget *w);
    static void setTextColor(QWidget *w, const QColor &c);

signals:
    void validChanged();
    void validChanged(bool validState);
    void validReturnPressed();

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const = 0;

protected slots:
    // Custom behaviour can be added here. The base implementation must
    // be called.
    virtual void slotReturnPressed();
    virtual void slotChanged(const QString &t);

private:
    BaseValidatingLineEditPrivate *m_bd;
};

} // namespace Utils

#endif // BASEVALIDATINGLINEEDIT_H
