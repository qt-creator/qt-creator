// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QtTaskTree/QTaskTree>

#include <utils/filepath.h>

// The console host the terminal runs its pseudo terminals through on Windows.
// The one that comes with Windows renders what an application writes into a
// text buffer and passes on what it can express itself, which drops images,
// and it is only updated along with the operating system. Microsoft publishes
// a newer one that passes such sequences through untouched.
namespace Terminal::Internal::ConsoleHost {

bool isSupportedPlatform();

QString version();
Utils::FilePath downloadDirectory();
bool isDownloaded();

// Where the console host in use comes from, empty for the one that comes with
// Windows.
Utils::FilePath inUse();

// Points the pseudo terminals at the console host to take, which is the one
// the settings name, the downloaded one, or the one Windows comes with.
void apply();

// Fetches and unpacks Microsoft's ConPTY package, after asking for the license.
QtTaskTree::GroupItem downloadRecipe();

QString dialogTitle();

} // namespace Terminal::Internal::ConsoleHost
