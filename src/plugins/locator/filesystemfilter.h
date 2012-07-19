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

#ifndef FILESYSTEMFILTER_H
#define FILESYSTEMFILTER_H

#include "ilocatorfilter.h"
#include "ui_filesystemfilter.h"

#include <QString>
#include <QList>
#include <QByteArray>
#include <QFutureInterface>

namespace Core {
    class EditorManager;
}

namespace Locator {
namespace Internal {

class LocatorWidget;

class FileSystemFilter : public Locator::ILocatorFilter
{
    Q_OBJECT

public:
    FileSystemFilter(Core::EditorManager *editorManager, LocatorWidget *locatorWidget);
    QString displayName() const { return tr("Files in File System"); }
    QString id() const { return QLatin1String("Files in file system"); }
    Locator::ILocatorFilter::Priority priority() const { return Locator::ILocatorFilter::Medium; }
    QList<Locator::FilterEntry> matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    bool openConfigDialog(QWidget *parent, bool &needsRefresh);
    void refresh(QFutureInterface<void> &) {}

private:
    Core::EditorManager *m_editorManager;
    LocatorWidget *m_locatorWidget;
    bool m_includeHidden;
};

} // namespace Internal
} // namespace Locator

#endif // FILESYSTEMFILTER_H
