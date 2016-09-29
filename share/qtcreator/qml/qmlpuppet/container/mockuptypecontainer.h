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

#include <qmetatype.h>
#include <QString>
#include <QDataStream>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class MockupTypeContainer
{
    friend QDataStream &operator>>(QDataStream &in, MockupTypeContainer &container);

public:
    MockupTypeContainer();
    MockupTypeContainer(const TypeName &type, const QString &importUri, int majorVersion, int minorVersion, bool isItem);

    TypeName typeName() const
    { return m_typeName; }

    QString importUri() const;

    int majorVersion() const
    { return m_majorVersion; }

    int minorVersion() const
    { return m_minorVersion; }

    bool isItem() const
    { return m_isItem; }

private:
    TypeName m_typeName;
    QString m_importUri;
    int m_majorVersion = -1;
    int m_minorVersion = -1;
    bool m_isItem = false;
};


QDataStream &operator<<(QDataStream &out, const MockupTypeContainer &container);
QDataStream &operator>>(QDataStream &in, MockupTypeContainer &container);

QDebug operator <<(QDebug debug, const MockupTypeContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::MockupTypeContainer)
