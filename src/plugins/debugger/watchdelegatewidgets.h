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

#ifndef WATCHDELEGATEEDITS_H
#define WATCHDELEGATEEDITS_H

#include <QtGui/QLineEdit>
#include <QtGui/QComboBox>
#include <QtCore/QVariant>

namespace Debugger {
namespace Internal {
class IntegerValidator;

/* Watch edit widgets. The logic is based on the QVariant 'modelData' property,
 * which is accessed by the WatchDelegate. */

/* WatchLineEdit: Base class for Watch delegate line edits with
 * ready-made accessors for the model's QVariants for QString-text use. */
class WatchLineEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText USER false)
    Q_PROPERTY(QVariant modelData READ modelData WRITE setModelData DESIGNABLE false USER true)
public:
    explicit WatchLineEdit(QWidget *parent = 0);

    // Ready-made accessors for item views passing QVariants around
    virtual QVariant modelData() const;
    virtual void setModelData(const QVariant &);

    static WatchLineEdit *create(QVariant::Type t, QWidget *parent = 0);
};

/* Watch delegate line edit for integer numbers based on quint64/qint64.
 * Does validation using the given number base (10, 16, 8, 2) and signedness.
 * isBigInt() indicates that no checking for number conversion is to be performed
 * (that is, value cannot be handled as quint64/qint64, for 128bit registers, etc). */
class IntegerWatchLineEdit : public WatchLineEdit
{
    Q_OBJECT
    Q_PROPERTY(int base READ base WRITE setBase DESIGNABLE true)
    Q_PROPERTY(bool Signed READ isSigned WRITE setSigned DESIGNABLE true)
    Q_PROPERTY(bool bigInt READ isBigInt WRITE setBigInt DESIGNABLE true)
public:
    explicit IntegerWatchLineEdit(QWidget *parent = 0);

    // Ready-made accessors for item views passing QVariants around
    virtual QVariant modelData() const;
    virtual void setModelData(const QVariant &);

    int base() const;
    void setBase(int b);
    bool isSigned() const;
    void setSigned(bool s);
    bool isBigInt() const;
    void setBigInt(bool b);

    static bool isUnsignedHexNumber(const QString &v);

private:
    void setNumberText(const QString &);
    inline QVariant modelDataI() const;
    IntegerValidator *m_validator;
};

/* Float line edit */
class FloatWatchLineEdit : public WatchLineEdit
{
public:
    explicit FloatWatchLineEdit(QWidget *parent = 0);

    virtual QVariant modelData() const;
    virtual void setModelData(const QVariant &);
};

/* Combo box for booleans */
class BooleanComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QVariant modelData READ modelData WRITE setModelData DESIGNABLE false USER true)
public:
    explicit BooleanComboBox(QWidget *parent = 0);

    virtual QVariant modelData() const;
    virtual void setModelData(const QVariant &);
};

} // namespace Internal
} // namespace Debugger

#endif // WATCHDELEGATEEDITS_H
