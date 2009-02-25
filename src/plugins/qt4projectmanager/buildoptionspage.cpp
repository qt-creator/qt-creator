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
