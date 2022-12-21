// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javascriptfilter.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QJSEngine>

namespace Core {
namespace Internal {

enum class EngineAction { Reset = 1, Abort };

JavaScriptFilter::JavaScriptFilter()
{
    setId("JavaScriptFilter");
    setDisplayName(tr("Evaluate JavaScript"));
    setDescription(tr("Evaluates arbitrary JavaScript expressions and copies the result."));
    setDefaultIncludedByDefault(false);
    setDefaultShortcutString("=");
    m_abortTimer.setSingleShot(true);
    m_abortTimer.setInterval(1000);
    connect(&m_abortTimer, &QTimer::timeout, this, [this] {
        m_aborted = true;
        if (m_engine)
            m_engine->setInterrupted(true);
    });
}

JavaScriptFilter::~JavaScriptFilter()
{
}

void JavaScriptFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)

    if (!m_engine)
        setupEngine();
    m_engine->setInterrupted(false);
    m_aborted = false;
    m_abortTimer.start();
}

QList<LocatorFilterEntry> JavaScriptFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)

    QList<LocatorFilterEntry> entries;
    if (entry.trimmed().isEmpty()) {
        entries.append({this, tr("Reset Engine"), QVariant::fromValue(EngineAction::Reset)});
    } else {
        const QString result = m_engine->evaluate(entry).toString();
        if (m_aborted) {
            const QString message = entry + " = " + tr("Engine aborted after timeout.");
            entries.append({this, message, QVariant::fromValue(EngineAction::Abort)});
        } else {
            const QString expression = entry + " = " + result;
            entries.append({this, expression, QVariant()});
            entries.append({this, tr("Copy to clipboard: %1").arg(result), result});
            entries.append({this, tr("Copy to clipboard: %1").arg(expression), expression});
        }
    }

    return entries;
}

void JavaScriptFilter::accept(const LocatorFilterEntry &selection, QString *newText,
                              int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)

    if (selection.internalData.isNull())
        return;

    const EngineAction action = selection.internalData.value<EngineAction>();
    if (action == EngineAction::Reset) {
        m_engine.reset();
        return;
    }
    if (action == EngineAction::Abort)
        return;

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(selection.internalData.toString());
}

void JavaScriptFilter::setupEngine()
{
    m_engine.reset(new QJSEngine);
    m_engine->evaluate(
                "function abs(x) { return Math.abs(x); }\n"
                "function acos(x) { return Math.acos(x); }\n"
                "function asin(x) { return Math.asin(x); }\n"
                "function atan(x) { return Math.atan(x); }\n"
                "function atan2(x, y) { return Math.atan2(x, y); }\n"
                "function bin(x) { return '0b' + x.toString(2); }\n"
                "function ceil(x) { return Math.ceil(x); }\n"
                "function cos(x) { return Math.cos(x); }\n"
                "function exp(x) { return Math.exp(x); }\n"
                "function e() { return Math.E; }\n"
                "function floor(x) { return Math.floor(x); }\n"
                "function hex(x) { return '0x' + x.toString(16); }\n"
                "function log(x) { return Math.log(x); }\n"
                "function max() { return Math.max.apply(null, arguments); }\n"
                "function min() { return Math.min.apply(null, arguments); }\n"
                "function oct(x) { return '0' + x.toString(8); }\n"
                "function pi() { return Math.PI; }\n"
                "function pow(x, y) { return Math.pow(x, y); }\n"
                "function random() { return Math.random(); }\n"
                "function round(x) { return Math.round(x); }\n"
                "function sin(x) { return Math.sin(x); }\n"
                "function sqrt(x) { return Math.sqrt(x); }\n"
                "function tan(x) { return Math.tan(x); }\n");
}

} // namespace Internal
} // namespace Core

Q_DECLARE_METATYPE(Core::Internal::EngineAction)
