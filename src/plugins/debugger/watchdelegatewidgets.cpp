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

#include "watchdelegatewidgets.h"

#include <QDoubleValidator>
#include <QDebug>

#include <utils/qtcassert.h>

enum { debug = 0 };

namespace Debugger {
namespace Internal {

// Basic watch line edit.
WatchLineEdit::WatchLineEdit(QWidget *parent)
    : QLineEdit(parent)
{}

QVariant WatchLineEdit::modelData() const
{
    return QVariant(text());
}

void WatchLineEdit::setModelData(const QVariant &v)
{
    if (debug)
        qDebug("WatchLineEdit::setModelData(%s, '%s')", v.typeName(), qPrintable(v.toString()));
    setText(v.toString());
}

 /* ------ IntegerWatchLineEdit helpers:
  *        Integer validator using different number bases. */
class IntegerValidator : public QValidator
{
public:
    explicit IntegerValidator(QObject *parent);
    virtual State validate(QString &, int &) const;

    int base() const           { return m_base; }
    void setBase(int b)        { m_base = b; }
    bool isSigned() const      { return m_signed; }
    void setSigned(bool s)     { m_signed = s; }
    bool isBigInt() const      { return m_bigInt; }
    void setBigInt(bool b)     { m_bigInt = b; }

    static State validateEntry(const QString &s, int base, bool signedV, bool bigInt);

private:
    static inline bool isCharAcceptable(const QChar &c, int base);

    int m_base;
    bool m_signed;
    bool m_bigInt;
};

IntegerValidator::IntegerValidator(QObject *parent) :
    QValidator(parent), m_base(10), m_signed(true), m_bigInt(false)
{
}

// Check valid digits depending on base.
bool IntegerValidator::isCharAcceptable(const QChar &c, int base)
{
    if (c.isLetter())
        return base == 16 && c.toLower().toLatin1() <= 'f';
    if (!c.isDigit())
        return false;
    const int digit = c.toLatin1() - '0';
    if (base == 8 && digit > 7)
        return false;
    if (base == 2 && digit > 1)
        return false;
    return true;
}

QValidator::State IntegerValidator::validate(QString &s, int &) const
{
    return IntegerValidator::validateEntry(s, m_base, m_signed, m_bigInt);
}

QValidator::State IntegerValidator::validateEntry(const QString &s, int base, bool signedV, bool bigInt)
{
    const int size = s.size();
    if (!size)
        return QValidator::Intermediate;
    int pos = 0;
    // Skip sign.
    if (signedV && s.at(pos) == QLatin1Char('-')) {
        pos++;
        if (pos == size)
            return QValidator::Intermediate;
    }
    // Hexadecimal: '0x'?
    if (base == 16 && pos + 2 <= size
        && s.at(pos) == QLatin1Char('0') && s.at(pos + 1) == QLatin1Char('x')) {
        pos+= 2;
        if (pos == size)
            return QValidator::Intermediate;
    }

    // Check characters past sign.
    for (; pos < size; pos++)
        if (!isCharAcceptable(s.at(pos), base))
            return QValidator::Invalid;
    // Check conversion unless big integer
    if (bigInt)
        return QValidator::Acceptable;
    bool ok;
    if (signedV)
        s.toLongLong(&ok, base);
    else
        s.toULongLong(&ok, base);
    return ok ? QValidator::Acceptable : QValidator::Intermediate;
}

IntegerWatchLineEdit::IntegerWatchLineEdit(QWidget *parent) :
    WatchLineEdit(parent),
    m_validator(new IntegerValidator(this))
{
    setValidator(m_validator);
}

bool IntegerWatchLineEdit::isUnsignedHexNumber(const QString &v)
{
    return IntegerValidator::validateEntry(v, 16, false, true) == QValidator::Acceptable;
}

int IntegerWatchLineEdit::base() const
{
    return m_validator->base();
}

void IntegerWatchLineEdit::setBase(int b)
{
    QTC_ASSERT(b, return);
    m_validator->setBase(b);
}

bool IntegerWatchLineEdit::isSigned() const
{
    return m_validator->isSigned();
}

void IntegerWatchLineEdit::setSigned(bool s)
{
    m_validator->setSigned(s);
}

bool IntegerWatchLineEdit::isBigInt() const
{
    return m_validator->isBigInt();
}

void IntegerWatchLineEdit::setBigInt(bool b)
{
    m_validator->setBigInt(b);
}

QVariant IntegerWatchLineEdit::modelDataI() const
{
    if (isBigInt()) // Big integer: Plain text
        return QVariant(text());
    bool ok;
    if (isSigned()) {
        const qint64 value = text().toLongLong(&ok, base());
        if (ok)
            return QVariant(value);
    } else {
        const quint64 value = text().toULongLong(&ok, base());
        if (ok)
            return QVariant(value);
    }
    return QVariant();
}

QVariant IntegerWatchLineEdit::modelData() const
{
    const QVariant data = modelDataI();
    if (debug)
        qDebug("IntegerLineEdit::modelData(): base=%d, signed=%d, bigint=%d returns %s '%s'",
               base(), isSigned(), isBigInt(), data.typeName(), qPrintable(data.toString()));
    return data;
}

void IntegerWatchLineEdit::setModelData(const QVariant &v)
{
    if (debug)
        qDebug(">IntegerLineEdit::setModelData(%s, '%s'): base=%d, signed=%d, bigint=%d",
               v.typeName(), qPrintable(v.toString()),
               base(), isSigned(), isBigInt());
    switch (v.type()) {
    case QVariant::Int:
    case QVariant::LongLong: {
        const qint64 iv = v.toLongLong();
        setSigned(true);
        setText(QString::number(iv, base()));
    }
        break;
    case QVariant::UInt:
    case QVariant::ULongLong: {
         const quint64 iv = v.toULongLong();
         setSigned(false);
         setText(QString::number(iv, base()));
        }
        break;
    case QVariant::ByteArray:
        setNumberText(QString::fromLatin1(v.toByteArray()));
        break;
    case QVariant::String:
        setNumberText(v.toString());
        break;
    default:
        qWarning("Invalid value (%s) passed to IntegerLineEdit::setModelData",
                 v.typeName());
        setText(QString(QLatin1Char('0')));
        break;
    }
    if (debug)
        qDebug("<IntegerLineEdit::setModelData(): base=%d, signed=%d, bigint=%d",
               base(), isSigned(), isBigInt());
}

void IntegerWatchLineEdit::setNumberText(const QString &t)
{
    setText(t);
}

// ------------- FloatWatchLineEdit
FloatWatchLineEdit::FloatWatchLineEdit(QWidget *parent) :
    WatchLineEdit(parent)
{
    setValidator(new QDoubleValidator(this));
}

QVariant FloatWatchLineEdit::modelData() const
{
    return QVariant(text().toDouble());
}

void FloatWatchLineEdit::setModelData(const QVariant &v)
{
    if (debug)
        qDebug("FloatWatchLineEdit::setModelData(%s, '%s')",
               v.typeName(), qPrintable(v.toString()));
    switch (v.type()) {
        break;
    case QVariant::Double:
    case QVariant::String:
        setText(v.toString());
        break;
    case QVariant::ByteArray:
        setText(QString::fromLatin1(v.toByteArray()));
        break;
    default:
        qWarning("Invalid value (%s) passed to FloatWatchLineEdit::setModelData",
                 v.typeName());
        setText(QString::number(0.0));
        break;
    }
}

WatchLineEdit *WatchLineEdit::create(QVariant::Type t, QWidget *parent)
{
    switch (t) {
    case QVariant::Bool:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
        return new IntegerWatchLineEdit(parent);
        break;
    case QVariant::Double:
        return new FloatWatchLineEdit(parent);
    default:
        break;
    }
    return new WatchLineEdit(parent);
}

BooleanComboBox::BooleanComboBox(QWidget *parent) : QComboBox(parent)
{
    QStringList items;
    items << QLatin1String("false") << QLatin1String("true");
    addItems(items);
}

QVariant BooleanComboBox::modelData() const
{
    // As not to confuse debuggers with 'true', 'false', we return integers 1,0.
    const int rc = currentIndex() == 1 ? 1 : 0;
    return QVariant(rc);
}

void BooleanComboBox::setModelData(const QVariant &v)
{
    if (debug)
        qDebug("BooleanComboBox::setModelData(%s, '%s')", v.typeName(), qPrintable(v.toString()));

    bool value = false;
    switch (v.type()) {
    case QVariant::Bool:
        value = v.toBool();
        break;
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
        value = v.toInt() != 0;
        break;
    default:
        qWarning("Invalid value (%s) passed to BooleanComboBox::setModelData", v.typeName());
        break;
    }
    setCurrentIndex(value ? 1 : 0);
}

} // namespace Internal
} // namespace Debugger
