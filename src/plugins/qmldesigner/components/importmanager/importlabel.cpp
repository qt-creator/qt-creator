/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "importlabel.h"
#include <coreplugin/coreicons.h>

#include <QHBoxLayout>
#include <QPushButton>
#include <QIcon>

namespace QmlDesigner {

ImportLabel::ImportLabel(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);


    m_removeButton = new QPushButton(this);
    m_removeButton->setIcon(Core::Icons::BUTTON_CLOSE.icon());
    m_removeButton->setFlat(true);
    m_removeButton->setMaximumWidth(20);
    m_removeButton->setMaximumHeight(20);
    m_removeButton->setFocusPolicy(Qt::NoFocus);
    m_removeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_removeButton->setToolTip(tr("Remove Import"));
    connect(m_removeButton, SIGNAL(clicked()), this, SLOT(emitRemoveImport()));
    layout->addWidget(m_removeButton);

    m_importLabel = new QLabel(this);
    layout->addWidget(m_importLabel);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void ImportLabel::setImport(const Import &import)
{
    m_importLabel->setText(import.toString(false));

    m_import = import;
}

const Import ImportLabel::import() const
{
    return m_import;
}

void ImportLabel::setReadOnly(bool readOnly) const
{
    m_removeButton->setDisabled(readOnly);
    m_removeButton->setIcon(readOnly ? QIcon()
                                     : Core::Icons::BUTTON_CLOSE.icon());
}

void ImportLabel::emitRemoveImport()
{
    emit removeImport(m_import);
}

} // namespace QmlDesigner
