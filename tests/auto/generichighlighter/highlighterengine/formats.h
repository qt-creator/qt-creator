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

#ifndef FORMATS_H
#define FORMATS_H

#include <QTextCharFormat>

class Formats
{
public:
    static Formats &instance();

    const QTextCharFormat &keywordFormat() const { return m_keywordFormat; }
    const QTextCharFormat &dataTypeFormat() const { return m_dataTypeFormat; }
    const QTextCharFormat &decimalFormat() const { return m_decimalFormat; }
    const QTextCharFormat &baseNFormat() const { return m_baseNFormat; }
    const QTextCharFormat &floatFormat() const { return m_floatFormat; }
    const QTextCharFormat &charFormat() const { return m_charFormat; }
    const QTextCharFormat &stringFormat() const { return m_stringFormat; }
    const QTextCharFormat &commentFormat() const { return m_commentFormat; }
    const QTextCharFormat &alertFormat() const { return m_alertFormat; }
    const QTextCharFormat &errorFormat() const { return m_errorFormat; }
    const QTextCharFormat &functionFormat() const { return m_functionFormat; }
    const QTextCharFormat &regionMarketFormat() const { return m_regionMarkerFormat; }
    const QTextCharFormat &othersFormat() const { return m_othersFormat; }

    QString name(const QTextCharFormat &format) const;

private:
    Formats();
    Q_DISABLE_COPY(Formats);

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_dataTypeFormat;
    QTextCharFormat m_decimalFormat;
    QTextCharFormat m_baseNFormat;
    QTextCharFormat m_floatFormat;
    QTextCharFormat m_charFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_alertFormat;
    QTextCharFormat m_errorFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_regionMarkerFormat;
    QTextCharFormat m_othersFormat;
};

#endif // FORMATS_H
