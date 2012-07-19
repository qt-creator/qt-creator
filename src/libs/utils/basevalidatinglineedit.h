/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef BASEVALIDATINGLINEEDIT_H
#define BASEVALIDATINGLINEEDIT_H

#include "utils_global.h"

#include <QLineEdit>

namespace Utils {

struct BaseValidatingLineEditPrivate;

class QTCREATOR_UTILS_EXPORT BaseValidatingLineEdit : public QLineEdit
{
    Q_OBJECT
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
    virtual QString fixInputString(const QString &string);

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
