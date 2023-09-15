// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "icontext.h"
#include "icore.h"

#include <utils/appmainwindow.h>

#include <functional>

QT_BEGIN_NAMESPACE
class QColor;
class QPrinter;
QT_END_NAMESPACE

namespace Utils { class InfoBar; }

namespace Core {

class IDocument;

namespace Internal {

class MainWindowPrivate;

class MainWindow : public Utils::AppMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

    void init();
    void extensionsInitialized();
    void aboutToShutdown();

    IContext *contextObject(QWidget *widget) const;
    void addContextObject(IContext *context);
    void removeContextObject(IContext *context);

    static IDocument *openFiles(const Utils::FilePaths &filePaths,
                                ICore::OpenFilesFlags flags,
                                const Utils::FilePath &workingDirectory = {});

    QPrinter *printer() const;
    IContext *currentContextObject() const;
    QStatusBar *statusBar() const;
    Utils::InfoBar *infoBar() const;

    void updateAdditionalContexts(const Context &remove, const Context &add,
                                  ICore::ContextPriority priority);

    void setOverrideColor(const QColor &color);

    QStringList additionalAboutInformation() const;
    void clearAboutInformation();
    void appendAboutInformation(const QString &line);

    void addPreCloseListener(const std::function<bool()> &listener);

    void saveSettings();

    void restart();

    void restartTrimmer();

public slots:
    static void openFileWith();
    void exit();

private:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

    MainWindowPrivate *d = nullptr;
};

} // namespace Internal
} // namespace Core
