/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilerattachdialog.h"

#include <projectexplorer/kitchooser.h>
#include <coreplugin/id.h>

#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>

using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

class QmlProfilerAttachDialogPrivate
{
public:
    QSpinBox *portSpinBox;
    KitChooser *kitChooser;
};

QmlProfilerAttachDialog::QmlProfilerAttachDialog(QWidget *parent) :
    QDialog(parent),
    d(new QmlProfilerAttachDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start QML Profiler"));

    d->kitChooser = new KitChooser(this);
    d->kitChooser->populate();

    d->portSpinBox = new QSpinBox(this);
    d->portSpinBox->setMaximum(65535);
    d->portSpinBox->setValue(3768);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(tr("Kit:"), d->kitChooser);
    formLayout->addRow(tr("&Port:"), d->portSpinBox);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QmlProfilerAttachDialog::~QmlProfilerAttachDialog()
{
    delete d;
}

int QmlProfilerAttachDialog::port() const
{
    return d->portSpinBox->value();
}

void QmlProfilerAttachDialog::setPort(const int port)
{
    d->portSpinBox->setValue(port);
}

Kit *QmlProfilerAttachDialog::kit() const
{
    return d->kitChooser->currentKit();
}

void QmlProfilerAttachDialog::setKitId(Core::Id id)
{
    d->kitChooser->setCurrentKitId(id);
}

} // namespace Internal
} // namespace QmlProfiler
