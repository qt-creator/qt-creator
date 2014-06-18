/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qtquickappwizardpages.h"

#include <utils/wizard.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

namespace QmakeProjectManager {
namespace Internal {

class QtQuickComponentSetPagePrivate
{
public:
    QComboBox *m_versionComboBox;
    QLabel *m_descriptionLabel;
};

QtQuickComponentSetPage::QtQuickComponentSetPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new QtQuickComponentSetPagePrivate)
{
    setTitle(tr("Select Qt Quick Component Set"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *l = new QHBoxLayout();

    QLabel *label = new QLabel(tr("Qt Quick component set:"), this);
    d->m_versionComboBox = new QComboBox(this);

    QSet<QString> availablePlugins;
    foreach (ExtensionSystem::PluginSpec *s, ExtensionSystem::PluginManager::plugins()) {
        if (s->state() == ExtensionSystem::PluginSpec::Running && !s->hasError())
              availablePlugins += s->name();
    }

    foreach (const TemplateInfo &templateInfo, QtQuickApp::templateInfos()) {
        bool ok = true;
        foreach (const QString &neededPlugin, templateInfo.requiredPlugins) {
            if (!availablePlugins.contains(neededPlugin)) {
                ok = false;
                break;
            }
        }
        if (ok)
            d->m_versionComboBox->addItem(templateInfo.displayName);
    }

    l->addWidget(label);
    l->addWidget(d->m_versionComboBox);

    d->m_descriptionLabel = new QLabel(this);
    d->m_descriptionLabel->setWordWrap(true);
    d->m_descriptionLabel->setTextFormat(Qt::RichText);
    connect(d->m_versionComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateDescription(int)));
    updateDescription(d->m_versionComboBox->currentIndex());

    mainLayout->addLayout(l);
    mainLayout->addWidget(d->m_descriptionLabel);

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Component Set"));
}

QtQuickComponentSetPage::~QtQuickComponentSetPage()
{
    delete d;
}

TemplateInfo QtQuickComponentSetPage::templateInfo() const
{
    if (QtQuickApp::templateInfos().isEmpty())
        return TemplateInfo();
    return QtQuickApp::templateInfos().at(d->m_versionComboBox->currentIndex());
}

void QtQuickComponentSetPage::updateDescription(int index)
{
    if (QtQuickApp::templateInfos().isEmpty())
        return;

    const TemplateInfo templateInfo = QtQuickApp::templateInfos().at(index);
    d->m_descriptionLabel->setText(templateInfo.description);
}

} // namespace Internal
} // namespace QmakeProjectManager
