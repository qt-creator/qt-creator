// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
