/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "richtexteditorproxy.h"
#include "richtexteditor.h"

#include <qmldesignerplugin.h>
#include <qmldesignerconstants.h>

#include <coreplugin/icore.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>

namespace QmlDesigner {

RichTextEditorProxy::RichTextEditorProxy(QObject *parent)
    : QObject(parent)
    , m_dialog(new QDialog(Core::ICore::dialogParent()))
    , m_widget(new RichTextEditor{})
{
    m_dialog->setModal(true);
    QGridLayout *layout = new QGridLayout{};

    layout->addWidget(m_widget);
    QDialogButtonBox *standardButtons = new QDialogButtonBox{QDialogButtonBox::Ok
                                                             | QDialogButtonBox::Cancel};

    connect(standardButtons, &QDialogButtonBox::accepted, m_dialog, &QDialog::accept);
    connect(standardButtons, &QDialogButtonBox::rejected, m_dialog, &QDialog::reject);

    layout->addWidget(standardButtons);

    m_dialog->setLayout(layout);

    connect(m_dialog, &QDialog::accepted, [this]() { emit accepted(); });
    connect(m_dialog, &QDialog::rejected, [this]() { emit rejected(); });
}

RichTextEditorProxy::~RichTextEditorProxy()
{
    delete m_dialog;
}

void RichTextEditorProxy::registerDeclarativeType()
{
    qmlRegisterType<RichTextEditorProxy>("HelperWidgets", 2, 0, "RichTextEditor");
}

void RichTextEditorProxy::showWidget()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_RICHTEXT_OPENED);
    m_dialog->show();
}

void RichTextEditorProxy::hideWidget()
{
    m_dialog->hide();
}

QString RichTextEditorProxy::richText() const
{
    return m_widget->richText();
}

void RichTextEditorProxy::setRichText(const QString &text)
{
    m_widget->setRichText(text);
}

} // namespace QmlDesigner
