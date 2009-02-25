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

#ifndef OPENDOCUMENTSFILTER_H
#define OPENDOCUMENTSFILTER_H

#include "iquickopenfilter.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QtCore/QFutureInterface>
#include <QtGui/QWidget>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

namespace QuickOpen {
namespace Internal {

class OpenDocumentsFilter : public QuickOpen::IQuickOpenFilter
{
    Q_OBJECT

public:
    OpenDocumentsFilter(Core::EditorManager *editorManager);
    QString trName() const { return tr("Open documents"); }
    QString name() const { return "Open documents"; }
    QuickOpen::IQuickOpenFilter::Priority priority() const { return QuickOpen::IQuickOpenFilter::Medium; }
    QList<QuickOpen::FilterEntry> matchesFor(const QString &entry);
    void accept(QuickOpen::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);

public slots:
    void refreshInternally();
signals:
    void invokeRefresh();
private:
    Core::EditorManager *m_editorManager;

    QList<Core::IEditor *> m_editors;
};

} // namespace Internal
} // namespace QuickOpen

#endif // OPENDOCUMENTSFILTER_H
