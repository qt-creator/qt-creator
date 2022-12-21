// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
