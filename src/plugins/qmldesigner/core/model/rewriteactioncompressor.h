/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef REWRITEACTIONCOMPRESSOR_H
#define REWRITEACTIONCOMPRESSOR_H

#include <QtCore/QStringList>

#include "rewriteaction.h"

namespace QmlDesigner {
namespace Internal {

class RewriteActionCompressor
{
public:
    RewriteActionCompressor(const QStringList &propertyOrder): m_propertyOrder(propertyOrder) {}

    void operator()(QList<RewriteAction *> &actions) const;

private:
    void compressImports(QList<RewriteAction *> &actions) const;

    void compressReparentActions(QList<RewriteAction *> &actions) const;
    void compressAddEditRemoveNodeActions(QList<RewriteAction *> &actions) const;
    void compressPropertyActions(QList<RewriteAction *> &actions) const;
    void compressAddEditActions(QList<RewriteAction *> &actions) const;
    void compressAddReparentActions(QList<RewriteAction *> &actions) const;

    void remove(QMutableListIterator<RewriteAction*> &iter) const;

private:
    QStringList m_propertyOrder;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // REWRITEACTIONCOMPRESSOR_H
