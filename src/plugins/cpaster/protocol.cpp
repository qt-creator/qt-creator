// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "protocol.h"

#include "cpastertr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVariant>

namespace CodePaster {

Protocol::Protocol(const ProtocolData &data)
    : m_protocolData(data)
{}

Protocol::~Protocol() = default;

QtTaskTree::ExecutableItem Protocol::fetchRecipe(const QString &, const FetchHandler &) const
{
    reportError(Tr::tr("Fetch cabability not supported."));
    return QtTaskTree::errorItem;
}

QtTaskTree::ExecutableItem Protocol::listRecipe(const ListHandler &) const
{
    reportError(Tr::tr("List cabability not supported."));
    return QtTaskTree::errorItem;
}

QtTaskTree::ExecutableItem Protocol::pasteRecipe(const PasteInputData &, const PasteHandler &) const
{
    reportError(Tr::tr("Paste cabability not supported."));
    return QtTaskTree::errorItem;
}

void Protocol::reportError(const QString &message) const
{
    Core::MessageManager::writeDisrupting(QString("%1: %2").arg(name(), message));
}

QString Protocol::fixNewLines(QString data)
{
    // Copied from cpaster. Otherwise lineendings will screw up
    // HTML requires "\r\n".
    if (data.contains(QLatin1String("\r\n")))
        return data;
    if (data.contains(QLatin1Char('\n'))) {
        data.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
        return data;
    }
    if (data.contains(QLatin1Char('\r')))
        data.replace(QLatin1Char('\r'), QLatin1String("\r\n"));
    return data;
}

// Show a configuration error and point user to settings.
static void showConfigurationError(const Protocol *p, const QString &message)
{
    const bool showConfig = p->settingsPage();

    const QString title = Tr::tr("%1 - Configuration Error").arg(p->name());
    QMessageBox mb(QMessageBox::Warning, title, message, QMessageBox::Cancel, Core::ICore::dialogParent());
    QPushButton *settingsButton = nullptr;
    if (showConfig)
        settingsButton = mb.addButton(Core::ICore::msgShowSettings(), QMessageBox::AcceptRole);
    mb.exec();
    if (mb.clickedButton() == settingsButton)
        Core::ICore::showSettings(p->settingsPage()->id());
}

bool Protocol::ensureConfiguration(Protocol *p)
{
    while (true) {
        const auto res = p->checkConfiguration();
        if (res)
            return true;
        // Cancel returns empty error message.
        if (res.error().isEmpty()) {
            showConfigurationError(p, res.error());
            break;
        }
    }
    return false;
}

} // CodePaster
