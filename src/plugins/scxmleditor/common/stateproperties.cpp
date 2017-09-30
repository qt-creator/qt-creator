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

#include "stateproperties.h"
#include "attributeitemdelegate.h"
#include "attributeitemmodel.h"
#include "scxmleditorconstants.h"
#include "scxmluifactory.h"

#include <coreplugin/minisplitter.h>
#include <utils/qtcfallthrough.h>

#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QPlainTextEdit>
#include <QTableView>
#include <QToolBar>

using namespace ScxmlEditor::PluginInterface;
using namespace ScxmlEditor::Common;

StateProperties::StateProperties(QWidget *parent)
    : QFrame(parent)
{
    createUi();

    m_contentFrame->setVisible(false);

    m_contentTimer.setInterval(500);
    m_contentTimer.setSingleShot(true);
    connect(m_contentEdit, &QPlainTextEdit::textChanged, &m_contentTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(&m_contentTimer, &QTimer::timeout, this, &StateProperties::timerTimeout);
}

void StateProperties::setCurrentTagName(const QString &tagName)
{
    QFontMetrics fontMetrics(font());
    m_currentTagName->setText(fontMetrics.elidedText(tagName, Qt::ElideRight, 100));
}

void StateProperties::updateName()
{
    QString tagName;
    if (m_tag) {
        if (m_tag->hasAttribute(Constants::C_SCXMLTAG_ATTRIBUTE_ID))
            tagName = m_tag->attribute(Constants::C_SCXMLTAG_ATTRIBUTE_ID);
        else if (m_tag->hasAttribute(Constants::C_SCXMLTAG_ATTRIBUTE_EVENT))
            tagName = m_tag->attribute(Constants::C_SCXMLTAG_ATTRIBUTE_EVENT);
        else
            tagName = m_tag->tagName();
    }
    setCurrentTagName(tagName);
}

void StateProperties::tagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value)
{
    Q_UNUSED(value)

    switch (change) {
    case ScxmlDocument::TagEditorInfoChanged:
    case ScxmlDocument::TagAttributesChanged:
    case ScxmlDocument::TagContentChanged:
        if (tag != m_tag)
            return;
        Q_FALLTHROUGH();
    case ScxmlDocument::TagCurrentChanged:
        setTag(tag);
        break;
    default:
        break;
    }
}

void StateProperties::setDocument(ScxmlDocument *document)
{
    if (m_document)
        disconnect(m_document, 0, this, 0);

    m_document = document;
    if (m_document) {
        m_tag = m_document->rootTag();
        connect(m_document, &ScxmlDocument::endTagChange, this, &StateProperties::tagChange);
    } else {
        setTag(0);
    }
}

void StateProperties::setUIFactory(ScxmlUiFactory *factory)
{
    m_uiFactory = factory;
    if (m_uiFactory) {
        m_attributeModel = static_cast<AttributeItemModel*>(m_uiFactory->object("attributeItemModel"));
        m_attributeDelegate = static_cast<AttributeItemDelegate*>(m_uiFactory->object("attributeItemDelegate"));

        m_tableView->setItemDelegate(m_attributeDelegate);
        m_tableView->setModel(m_attributeModel);
    }
}

void StateProperties::createUi()
{
    auto titleLabel = new QLabel(tr("Attributes"));
    titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    m_currentTagName = new QLabel;

    auto propertiesToolBar = new QToolBar;
    propertiesToolBar->setMinimumHeight(24);
    propertiesToolBar->addWidget(titleLabel);
    propertiesToolBar->addWidget(m_currentTagName);

    m_tableView = new QTableView;
    m_tableView->setEditTriggers(QAbstractItemView::CurrentChanged
                                 | QAbstractItemView::DoubleClicked
                                 | QAbstractItemView::SelectedClicked
                                 | QAbstractItemView::EditKeyPressed
                                 | QAbstractItemView::AnyKeyPressed);
    m_tableView->setFrameShape(QFrame::NoFrame);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);

    m_contentEdit = new QPlainTextEdit;

    m_contentFrame = new QWidget;
    m_contentFrame->setLayout(new QVBoxLayout);
    m_contentFrame->layout()->addWidget(new QLabel(tr("Content")));
    m_contentFrame->layout()->addWidget(m_contentEdit);

    auto splitter = new Core::MiniSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(m_tableView);
    splitter->addWidget(m_contentFrame);

    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    layout()->setSpacing(0);
    layout()->addWidget(propertiesToolBar);
    layout()->addWidget(splitter);
}

void StateProperties::setTag(ScxmlTag *tag)
{
    m_tag = tag;
    m_attributeDelegate->setTag(m_tag);
    m_attributeModel->setTag(m_tag);
    setContentVisibility(m_tag && m_tag->info()->canIncludeContent);
    updateName();
}

void StateProperties::timerTimeout()
{
    if (m_tag && m_document && m_tag->info()->canIncludeContent && m_tag->content() != m_contentEdit->toPlainText())
        m_document->setContent(m_tag, m_contentEdit->toPlainText());
}

QString StateProperties::content() const
{
    if (m_tag && m_tag->info()->canIncludeContent)
        return m_tag->content();

    return QString();
}

void StateProperties::setContentVisibility(bool visible)
{
    m_contentFrame->setVisible(visible);
    updateContent();
}

void StateProperties::updateContent()
{
    if (!m_contentEdit->hasFocus()) {
        QSignalBlocker blocker(m_contentEdit);
        m_contentEdit->setPlainText(content());
    }
}
