/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QUICKOPENFILTERSFILTER_H
#define QUICKOPENFILTERSFILTER_H

#include "iquickopenfilter.h"

#include <QtGui/QIcon>

namespace QuickOpen {
namespace Internal {

class QuickOpenPlugin;
class QuickOpenToolWindow;

/*!
  This filter provides the user with the list of available QuickOpen filters.
  The list is only shown when nothing has been typed yet.
 */
class QuickOpenFiltersFilter : public IQuickOpenFilter
{
public:
    QuickOpenFiltersFilter(QuickOpenPlugin *plugin,
                           QuickOpenToolWindow *toolWindow);

    // IQuickOpenFilter
    QString trName() const;
    QString name() const;
    Priority priority() const;
    QList<FilterEntry> matchesFor(const QString &entry);
    void accept(FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);
    bool isConfigurable() const;

private:
    QuickOpenPlugin *m_plugin;
    QuickOpenToolWindow *m_toolWindow;
    QIcon m_icon;
};

} // namespace Internal
} // namespace QuickOpen

#endif // QUICKOPENFILTERSFILTER_H
