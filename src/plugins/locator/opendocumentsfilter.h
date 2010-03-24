/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef OPENDOCUMENTSFILTER_H
#define OPENDOCUMENTSFILTER_H

#include "ilocatorfilter.h"

#include <coreplugin/editormanager/openeditorsmodel.h>

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QtCore/QFutureInterface>

namespace Core {
    class EditorManager;
}

namespace Locator {
namespace Internal {

class OpenDocumentsFilter : public Locator::ILocatorFilter
{
    Q_OBJECT

public:
    explicit OpenDocumentsFilter(Core::EditorManager *editorManager);
    QString displayName() const { return tr("Open documents"); }
    QString id() const { return "Open documents"; }
    Locator::ILocatorFilter::Priority priority() const { return Locator::ILocatorFilter::Medium; }
    QList<Locator::FilterEntry> matchesFor(const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);

public slots:
    void refreshInternally();

private:
    Core::EditorManager *m_editorManager;

    QList<Core::OpenEditorsModel::Entry> m_editors;
};

} // namespace Internal
} // namespace Locator

#endif // OPENDOCUMENTSFILTER_H
