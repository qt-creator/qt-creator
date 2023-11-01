// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "consoleitem.h"

#include <coreplugin/ioutputpane.h>
#include <utils/aspects.h>

#include <functional>

#include <QObject>

QT_BEGIN_NAMESPACE
class QToolButton;
class QLabel;
QT_END_NAMESPACE

namespace Utils { class BoolAspect; }

namespace Debugger::Internal {

using ScriptEvaluator = std::function<void (QString)>;

class ConsoleItemModel;
class ConsoleView;

class Console final : public Core::IOutputPane
{
public:
    Console();
    ~Console() override;

    QWidget *outputWidget(QWidget *) override;
    QList<QWidget *> toolBarWidgets() const override;
    void clearContents() override;
    bool canFocus() const override;
    bool hasFocus() const override;
    void setFocus() override;

    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;
    bool canNavigate() const override;

    void readSettings();
    void setContext(const QString &context);

    void setScriptEvaluator(const ScriptEvaluator &evaluator);
    void populateFileFinder();

    void evaluate(const QString &expression);
    void printItem(ConsoleItem *item);
    void printItem(ConsoleItem::ItemType itemType, const QString &text);

    void writeSettings() const;

private:
    QToolButton *m_showDebugButton;
    QToolButton *m_showWarningButton;
    QToolButton *m_showErrorButton;
    Utils::BoolAspect m_showDebug;
    Utils::BoolAspect m_showWarning;
    Utils::BoolAspect m_showError;
    QWidget *m_spacer;
    QLabel *m_statusLabel;
    ConsoleItemModel *m_consoleItemModel;
    ConsoleView *m_consoleView;
    QWidget *m_consoleWidget;
    ScriptEvaluator m_scriptEvaluator;
};

Console *debuggerConsole();

} // Debugger::Internal
