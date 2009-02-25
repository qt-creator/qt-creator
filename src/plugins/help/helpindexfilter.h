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

#ifndef HELPINDEXFILTER_H
#define HELPINDEXFILTER_H

#include <quickopen/iquickopenfilter.h>

#include <QtGui/QIcon>

QT_BEGIN_NAMESPACE
class QHelpEngine;
class QUrl;
QT_END_NAMESPACE

namespace Help {
namespace Internal {

class HelpPlugin;

class HelpIndexFilter : public QuickOpen::IQuickOpenFilter
{
    Q_OBJECT

public:
    HelpIndexFilter(HelpPlugin *plugin, QHelpEngine *helpEngine);

    // IQuickOpenFilter
    QString trName() const;
    QString name() const;
    Priority priority() const;
    QList<QuickOpen::FilterEntry> matchesFor(const QString &entry);
    void accept(QuickOpen::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);

signals:
    void linkActivated(const QUrl &link) const;
    void linksActivated(const QMap<QString, QUrl> &urls, const QString &keyword) const;

private slots:
    void updateIndices();

private:
    HelpPlugin *m_plugin;
    QHelpEngine *m_helpEngine;
    QStringList m_helpIndex;
    QIcon m_icon;
};

} // namespace Internal
} // namespace Help

#endif // HELPINDEXFILTER_H
