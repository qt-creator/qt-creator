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

#pragma once

#include "scxmldocument.h"
#include "scxmltag.h"

#include <QFrame>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)
QT_FORWARD_DECLARE_CLASS(QTableView)

namespace ScxmlEditor {

namespace PluginInterface {
class AttributeItemModel;
class AttributeItemDelegate;
class ScxmlUiFactory;
} // namespace PluginInterface

namespace Common {

/**
 * @brief The StateProperties class provides the view to show/edit attributes of the selected tag.
 */
class StateProperties : public QFrame
{
    Q_OBJECT

public:
    explicit StateProperties(QWidget *parent = nullptr);

    void setDocument(PluginInterface::ScxmlDocument *document);
    void setUIFactory(PluginInterface::ScxmlUiFactory *factory);

private:
    void createUi();
    void setCurrentTagName(const QString &state);
    void tagChange(PluginInterface::ScxmlDocument::TagChange change, PluginInterface::ScxmlTag *tag, const QVariant &value);
    void timerTimeout();
    void setContentVisibility(bool visible);
    void updateContent();
    void updateName();
    void setTag(PluginInterface::ScxmlTag *tag);
    QString content() const;

    PluginInterface::AttributeItemModel *m_attributeModel = nullptr;
    PluginInterface::AttributeItemDelegate *m_attributeDelegate = nullptr;
    PluginInterface::ScxmlDocument *m_document = nullptr;
    PluginInterface::ScxmlTag *m_tag = nullptr;
    QTimer m_contentTimer;
    QPointer<PluginInterface::ScxmlUiFactory> m_uiFactory;

    QWidget *m_contentFrame = nullptr;
    QLabel *m_currentTagName = nullptr;
    QPlainTextEdit *m_contentEdit = nullptr;
    QTableView *m_tableView = nullptr;
};

} // namespace Common
} // namespace ScxmlEditor
