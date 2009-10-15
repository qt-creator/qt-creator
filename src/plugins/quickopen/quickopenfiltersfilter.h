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

#ifndef QUICKOPENFILTERSFILTER_H
#define QUICKOPENFILTERSFILTER_H

#include "ilocatorfilter.h"

#include <QtGui/QIcon>

namespace QuickOpen {
namespace Internal {

class QuickOpenPlugin;
class LocatorWidget;

/*!
  This filter provides the user with the list of available QuickOpen filters.
  The list is only shown when nothing has been typed yet.
 */
class QuickOpenFiltersFilter : public ILocatorFilter
{
    Q_OBJECT

public:
    QuickOpenFiltersFilter(QuickOpenPlugin *plugin,
                           LocatorWidget *locatorWidget);

    // ILocatorFilter
    QString trName() const;
    QString name() const;
    Priority priority() const;
    QList<FilterEntry> matchesFor(const QString &entry);
    void accept(FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);
    bool isConfigurable() const;

private:
    QuickOpenPlugin *m_plugin;
    LocatorWidget *m_locatorWidget;
    QIcon m_icon;
};

} // namespace Internal
} // namespace QuickOpen

#endif // QUICKOPENFILTERSFILTER_H
