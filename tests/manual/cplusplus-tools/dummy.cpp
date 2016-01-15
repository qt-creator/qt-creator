/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "dummy.h"
#include "detail/header.h"

using namespace test;

extern int xi;

Dummy::Dummy()
{}

Dummy::Dummy(int)
{
    xi = 0;
    freefunc2(1.0);
}

void Dummy::bla(int)
{}

void Dummy::bla(const QString &)
{}

void Dummy::bla(const QString &) const
{}

void Dummy::bla(int, const QString &) const
{}

void Dummy::sfunc()
{}

const double Dummy::PI = 3.14;
