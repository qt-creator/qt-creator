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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlapplicationwizardpages.h"
#include "qmlapp.h"

#include <utils/wizard.h>

#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

namespace QmlProjectManager {
namespace Internal {

class QmlComponentSetPagePrivate
{
public:
    QComboBox *m_versionComboBox;
    QLabel *m_detailedDescriptionLabel;
};

QmlComponentSetPage::QmlComponentSetPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new QmlComponentSetPagePrivate)
{
    setTitle(tr("Select Qt Quick Component Set"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *l = new QHBoxLayout();

    QLabel *label = new QLabel(tr("Qt Quick component set:"), this);
    d->m_versionComboBox = new QComboBox(this);

    foreach (const TemplateInfo &templateInfo, QmlApp::templateInfos())
        d->m_versionComboBox->addItem(templateInfo.displayName);

    l->addWidget(label);
    l->addWidget(d->m_versionComboBox);

    d->m_detailedDescriptionLabel = new QLabel(this);
    d->m_detailedDescriptionLabel->setWordWrap(true);
    d->m_detailedDescriptionLabel->setTextFormat(Qt::RichText);
    connect(d->m_versionComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateDescription(int)));
    updateDescription(d->m_versionComboBox->currentIndex());

    mainLayout->addLayout(l);
    mainLayout->addWidget(d->m_detailedDescriptionLabel);

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Component Set"));
}

QmlComponentSetPage::~QmlComponentSetPage()
{
    delete d;
}

TemplateInfo QmlComponentSetPage::templateInfo() const
{
    if (QmlApp::templateInfos().isEmpty())
        return TemplateInfo();
    return QmlApp::templateInfos().at(d->m_versionComboBox->currentIndex());
}

void QmlComponentSetPage::updateDescription(int index)
{
    if (QmlApp::templateInfos().isEmpty())
        return;

    const TemplateInfo templateInfo = QmlApp::templateInfos().at(index);
    d->m_detailedDescriptionLabel->setText(templateInfo.description);
}

} // namespace Internal
} // namespace QmlProjectManager
