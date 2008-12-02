/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef FILESYSTEMFILTER_H
#define FILESYSTEMFILTER_H

#include "iquickopenfilter.h"
#include "ui_filesystemfilter.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QtCore/QFutureInterface>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace QuickOpen {
namespace Internal {

class QuickOpenToolWindow;

class FileSystemFilter : public QuickOpen::IQuickOpenFilter
{
    Q_OBJECT

public:
    FileSystemFilter(Core::EditorManager *editorManager, QuickOpenToolWindow *toolWindow);
    QString trName() const { return tr("File in file system"); }
    QString name() const { return "File in file system"; }
    QuickOpen::IQuickOpenFilter::Priority priority() const { return QuickOpen::IQuickOpenFilter::Medium; }
    QList<QuickOpen::FilterEntry> matchesFor(const QString &entry);
    void accept(QuickOpen::FilterEntry selection) const;
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    bool openConfigDialog(QWidget *parent, bool &needsRefresh);
    void refresh(QFutureInterface<void> &) {}

private:
    Core::EditorManager *m_editorManager;
    QuickOpenToolWindow *m_toolWindow;
    bool m_includeHidden;
};

} // namespace Internal
} // namespace QuickOpen

#endif // FILESYSTEMFILTER_H
