/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef BASEFILEFILTER_H
#define BASEFILEFILTER_H

#include "quickopen_global.h"
#include "iquickopenfilter.h"

#include <coreplugin/icore.h>

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QtGui/QWidget>

namespace QuickOpen {

class QUICKOPEN_EXPORT BaseFileFilter : public QuickOpen::IQuickOpenFilter
{
    Q_OBJECT

public:
    BaseFileFilter(Core::ICore *core);
    QList<QuickOpen::FilterEntry> matchesFor(const QString &entry);
    void accept(QuickOpen::FilterEntry selection) const;

protected:
    void generateFileNames();

    Core::ICore *m_core;
    QStringList m_files;
    QStringList m_fileNames;
    QStringList m_previousResultPaths;
    QStringList m_previousResultNames;
    bool m_forceNewSearchList;
    QString m_previousEntry;
};

} // namespace QuickOpen

#endif // BASEFILEFILTER_H
