/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "ui_qtquickcomponentsetoptionspage.h"
#include <QComboBox>

namespace QmakeProjectManager {
namespace Internal {

class QtQuickComponentSetPagePrivate
{
public:
    QComboBox *m_versionComboBox;
    QLabel *m_descriptionLabel;
};

QString QtQuickComponentSetPage::description(QtQuickApp::ComponentSet componentSet) const
{
    const QString basicDescription = tr("Creates a Qt Quick 1 application project that can contain "
                                        "both QML and C++ code and includes a QDeclarativeView.<br><br>");
    const QString basicDescription2 = tr("Creates a Qt Quick 2 application project that can contain "
                                        "both QML and C++ code and includes a QQuickView.<br><br>");
    switch (componentSet) {
    case QtQuickApp::QtQuickControls10:
        return basicDescription2 +  tr("Creates a deployable Qt Quick application using "
                                      "Qt Quick Controls. All files and directories that "
                                      "reside in the same directory as the main .qml file "
                                      "are deployed. You can modify the contents of the "
                                      "directory any time before deploying.\n\nRequires <b>Qt 5.1</b> or newer.");
    case QtQuickApp::QtQuick20Components:
        return basicDescription2 + tr("The built-in QML types in the QtQuick 2 namespace allow "
                                                            "you to write cross-platform applications with "
                                                            "a custom look and feel.\n\nRequires <b>Qt 5.0</b> or newer.");
    case QtQuickApp::QtQuick10Components:
        return basicDescription + tr("The built-in QML types in the QtQuick 1 namespace allow "
                                     "you to write cross-platform applications with "
                                     "a custom look and feel.\n\nRequires <b>Qt 4.8</b> or newer.");
    }
    return QString();
}

QtQuickComponentSetPage::QtQuickComponentSetPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new QtQuickComponentSetPagePrivate)
{
    setTitle(tr("Select Qt Quick Component Set"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *l = new QHBoxLayout();

    QLabel *label = new QLabel(tr("Qt Quick component set:"), this);
    d->m_versionComboBox = new QComboBox(this);
    d->m_versionComboBox->addItem(tr("Qt Quick Controls 1.0"), QtQuickApp::QtQuickControls10);
    d->m_versionComboBox->addItem(tr("Qt Quick 2.0"), QtQuickApp::QtQuick20Components);
    d->m_versionComboBox->addItem(tr("Qt Quick 1.1"), QtQuickApp::QtQuick10Components);

    l->addWidget(label);
    l->addWidget(d->m_versionComboBox);

    d->m_descriptionLabel = new QLabel(this);
    d->m_descriptionLabel->setWordWrap(true);
    d->m_descriptionLabel->setTextFormat(Qt::RichText);
    connect(d->m_versionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDescription(int)));
    updateDescription(d->m_versionComboBox->currentIndex());

    mainLayout->addLayout(l);
    mainLayout->addWidget(d->m_descriptionLabel);
}

QtQuickComponentSetPage::~QtQuickComponentSetPage()
{
    delete d;
}

QtQuickApp::ComponentSet QtQuickComponentSetPage::componentSet(int index) const
{
    return (QtQuickApp::ComponentSet)d->m_versionComboBox->itemData(index).toInt();
}

QtQuickApp::ComponentSet QtQuickComponentSetPage::componentSet() const
{
    return componentSet(d->m_versionComboBox->currentIndex());
}

void QtQuickComponentSetPage::updateDescription(int index)
{
    d->m_descriptionLabel->setText(description(componentSet(index)));
}

} // namespace Internal
} // namespace QmakeProjectManager
