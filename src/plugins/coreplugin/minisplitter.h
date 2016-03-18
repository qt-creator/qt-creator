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

#pragma once

#include "core_global.h"

#include <QSplitter>

QT_BEGIN_NAMESPACE
class QSplitterHandle;
QT_END_NAMESPACE

namespace Core {

/*! This is a simple helper-class to obtain mac-style 1-pixel wide splitters */
class CORE_EXPORT MiniSplitter : public QSplitter
{
public:
    enum SplitterStyle {Dark, Light};

    MiniSplitter(QWidget *parent = 0, SplitterStyle style = Dark);
    MiniSplitter(Qt::Orientation orientation, SplitterStyle style = Dark);

protected:
    QSplitterHandle *createHandle();

private:
    SplitterStyle m_style;
};

} // namespace Core
