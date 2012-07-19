/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef LINENUMBERFILTER_H
#define LINENUMBERFILTER_H

#include <locator/ilocatorfilter.h>

#include <QString>
#include <QList>
#include <QFutureInterface>

namespace TextEditor {

class ITextEditor;

namespace Internal {

class LineNumberFilter : public Locator::ILocatorFilter
{
    Q_OBJECT

public:
    explicit LineNumberFilter(QObject *parent = 0);

    QString displayName() const { return tr("Line in Current Document"); }
    QString id() const { return QLatin1String("Line in current document"); }
    Locator::ILocatorFilter::Priority priority() const { return Locator::ILocatorFilter::High; }
    QList<Locator::FilterEntry> matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &) {}

private:
    ITextEditor *currentTextEditor() const;
};

} // namespace Internal
} // namespace TextEditor

#endif // LINENUMBERFILTER_H
