/****************************************************************************
**
** Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
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

#include "javascriptfilter.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QJSEngine>

namespace Core {
namespace Internal {

enum JavaScriptAction
{
    ResetEngine = QVariant::UserType + 1,
    AbortEngine
};

JavaScriptFilter::JavaScriptFilter()
{
    setId("JavaScriptFilter");
    setDisplayName(tr("Evaluate JavaScript"));
    setIncludedByDefault(false);
    setShortcutString("=");
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
        entries.append({this, tr("Reset Engine"), QVariant(ResetEngine, nullptr)});
    } else {
        const QString result = m_engine->evaluate(entry).toString();
        if (m_aborted) {
            const QString message = entry + " = " + tr("Engine aborted after timeout.");
            entries.append({this, message, QVariant(AbortEngine, nullptr)});
        } else {
            const QString expression = entry + " = " + result;
            entries.append({this, expression, QVariant()});
            entries.append({this, tr("Copy to clipboard: %1").arg(result), result});
            entries.append({this, tr("Copy to clipboard: %1").arg(expression), expression});
        }
    }

    return entries;
}

void JavaScriptFilter::accept(Core::LocatorFilterEntry selection, QString *newText,
                              int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)

    if (selection.internalData.isNull())
        return;

    if (selection.internalData.userType() == ResetEngine) {
        m_engine.reset();
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(selection.internalData.toString());
}

void JavaScriptFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
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
