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

#ifndef SUBVERSIONOUTPUTWINDOW_H
#define SUBVERSIONOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

namespace Subversion {
namespace Internal {

class SubversionPlugin;

class SubversionOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    SubversionOutputWindow(SubversionPlugin *svnPlugin);
    ~SubversionOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets(void) const {
        return QList<QWidget *>();
    }

    QString name() const;
    void clearContents();
    int priorityInStatusBar() const;
    void visibilityChanged(bool visible);

    virtual bool canFocus();
    virtual bool hasFocus();
    virtual void setFocus();

public slots:
    void append(const QString &txt, bool popup = false);

private:

    SubversionPlugin *m_svnPlugin;
    QListWidget *m_outputListWidget;
};

} // namespace Subversion
} // namespace Internal

#endif // SUBVERSIONOUTPUTWINDOW_H
