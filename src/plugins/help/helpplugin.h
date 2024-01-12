// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/helpmanager.h>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Help::Internal {

class HelpWidget;
class HelpViewer;

void showHelpUrl(const QUrl &url, Core::HelpManager::HelpViewerLocation location);
void showLinksInCurrentViewer(const QMultiMap<QString, QUrl> &links,
                              const QString &key);
HelpViewer *createHelpViewer();
HelpWidget *modeHelpWidget();

} // Help::Internal
