/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef BASEVALIDATINGLINEEDIT_H
#define BASEVALIDATINGLINEEDIT_H

#include "utils_global.h"

#include <QtGui/QLineEdit>

namespace Core {
namespace Utils {

struct BaseValidatingLineEditPrivate;

/* Base class for validating line edits that performs validation in a virtual
 * validate() function to be implemented in derived classes.
 * When invalid, the text color will turn red and a tooltip will
 * contain the error message. This approach is less intrusive than a
 * QValidator which will prevent the user from entering certain characters.
 *
 * The widget has a concept of an "initialText" which can be something like
 * "<Enter name here>". This results in state 'DisplayingInitialText', which
 * is not valid, but is not marked red. */

class QWORKBENCH_UTILS_EXPORT BaseValidatingLineEdit : public QLineEdit {
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

    static QColor textColor(const QWidget *w);
    static void setTextColor(QWidget *w, const QColor &c);

signals:
    void validChanged();
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
} // namespace Core
#endif // BASEVALIDATINGLINEEDIT_H
