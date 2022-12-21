// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
