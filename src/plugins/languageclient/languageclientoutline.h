// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/lsptypes.h>
#include <texteditor/ioutlinewidget.h>
#include <utils/treemodel.h>

namespace TextEditor {
class TextDocument;
class BaseTextEditor;
} // namespace TextEditor
namespace Utils { class TreeViewComboBox; }

namespace LanguageClient {
class Client;

class LANGUAGECLIENT_EXPORT LanguageClientOutlineItem
    : public Utils::TypedTreeItem<LanguageClientOutlineItem>
{
public:
    LanguageClientOutlineItem() = default;
    LanguageClientOutlineItem(const LanguageServerProtocol::SymbolInformation &info);
    LanguageClientOutlineItem(Client *client, const LanguageServerProtocol::DocumentSymbol &info);

    LanguageServerProtocol::Range range() const { return m_range; }
    LanguageServerProtocol::Range selectionRange() const { return m_selectionRange; }
    LanguageServerProtocol::Position pos() const { return m_range.start(); }
    bool contains(const LanguageServerProtocol::Position &pos) const {
        return m_range.contains(pos);
    }

protected:
    // TreeItem interface
    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;

    QString name() const { return m_name; }
    QString detail() const { return m_detail; }
    int type() const { return m_type; }

private:
    Client * const m_client = nullptr;
    QString m_name;
    QString m_detail;
    LanguageServerProtocol::Range m_range;
    LanguageServerProtocol::Range m_selectionRange;
    int m_type = -1;
};

class Client;

class LanguageClientOutlineWidgetFactory : public TextEditor::IOutlineWidgetFactory
{
public:
    using IOutlineWidgetFactory::IOutlineWidgetFactory;

    static Utils::TreeViewComboBox *createComboBox(Client *client, TextEditor::BaseTextEditor *editor);
    // IOutlineWidgetFactory interface
public:
    bool supportsEditor(Core::IEditor *editor) const override;
    TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor) override;
    bool supportsSorting() const override { return true; }
};

} // namespace LanguageClient
