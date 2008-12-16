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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "buildoptionspage.h"

#include <QtCore/QSettings>
#include <QtGui/QLineEdit>

BuildOptionsPage::BuildOptionsPage(QWorkbench::PluginManager *app)
{
    core = app;
    QWorkbench::ICore *coreIFace = core->interface<QWorkbench::ICore>();
    if (coreIFace && coreIFace->settings()) {
        QSettings *s = coreIFace->settings();
        s->beginGroup("BuildOptions");
        m_qmakeCmd = s->value("QMake", "qmake").toString();
        m_makeCmd = s->value("Make", "make").toString();
        m_makeCleanCmd = s->value("MakeClean", "make clean").toString();
        s->endGroup();
    }    
}

QString BuildOptionsPage::name() const
{
    return tr("Commands");
}

QString BuildOptionsPage::category() const
{
    return "Qt4|Build";
}

QString BuildOptionsPage::trCategory() const
{
    return tr("Qt4|Build");
}

QWidget *BuildOptionsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_ui.qmakeLineEdit->setText(m_qmakeCmd);
    m_ui.makeLineEdit->setText(m_makeCmd);
    m_ui.makeCleanLineEdit->setText(m_makeCleanCmd);
    
    return w;
}

void BuildOptionsPage::finished(bool accepted)
{
	if (!accepted)
		return;

    m_qmakeCmd = m_ui.qmakeLineEdit->text();
    m_makeCmd = m_ui.makeLineEdit->text();
    m_makeCleanCmd = m_ui.makeCleanLineEdit->text();
    QWorkbench::ICore *coreIFace = core->interface<QWorkbench::ICore>();
    if (coreIFace && coreIFace->settings()) {
        QSettings *s = coreIFace->settings();
        s->beginGroup("BuildOptions");
        s->setValue("QMake", m_qmakeCmd);
        s->setValue("Make", m_makeCmd);
        s->setValue("MakeClean", m_makeCleanCmd);
        s->endGroup();
    }
}

QWorkbench::Plugin *BuildOptionsPage::plugin()
{
	return 0;
}

QObject *BuildOptionsPage::qObject()
{
	return this;
}
