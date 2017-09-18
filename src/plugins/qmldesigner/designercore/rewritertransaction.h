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

#include <qmldesignercorelib_global.h>

#include <QPointer>

namespace QmlDesigner {

class AbstractView;

class QMLDESIGNERCORE_EXPORT RewriterTransaction
{
public:
    RewriterTransaction();
    RewriterTransaction(AbstractView *view, const QByteArray &identifier);
    ~RewriterTransaction();
    void commit();
    void rollback();
    RewriterTransaction(const RewriterTransaction &other);
    RewriterTransaction& operator=(const RewriterTransaction &other);

    bool isValid() const;

    void ignoreSemanticChecks();

protected:
    AbstractView *view();
private:
   QPointer<AbstractView> m_view;
   QByteArray m_identifier;
   mutable bool m_valid;
   int m_identifierNumber = 0;
   static QList<QByteArray> m_identifierList;
   static bool m_activeIdentifier;
   bool m_ignoreSemanticChecks = false;
};

} //QmlDesigner
