// Copyright (C) 2018 Benjamin Balga
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "serialterminalsettings.h"
#include "serialterminalconstants.h"
#include "serialterminaltr.h"

#include <utils/qtcsettings.h>

#include <QLoggingCategory>

using namespace Utils;

namespace SerialTerminal {
namespace Internal {

static Q_LOGGING_CATEGORY(log, Constants::LOGGING_CATEGORY, QtWarningMsg)

// Set 'value' only if the key exists in the settings
template <typename T>
void readSetting(const QtcSettings &settings, T &value, const Key &key) {
    if (settings.contains(key))
        value = settings.value(key).value<T>();
}

Settings::Settings()
{
    lineEndings = {
        {Tr::tr("None"), ""},
        {Tr::tr("LF"), "\n"},
        {Tr::tr("CR"), "\r"},
        {Tr::tr("CRLF"), "\r\n"}
    };
    defaultLineEndingIndex = 1;
}

// Save settings to a QSettings
void Settings::save(QtcSettings *settings)
{
    if (!settings || !edited)
        return;

    settings->beginGroup(Constants::SETTINGS_GROUP);

    settings->setValue(Constants::SETTINGS_BAUDRATE, baudRate);
    settings->setValue(Constants::SETTINGS_DATABITS, dataBits);
    settings->setValue(Constants::SETTINGS_PARITY, parity);
    settings->setValue(Constants::SETTINGS_STOPBITS, stopBits);
    settings->setValue(Constants::SETTINGS_FLOWCONTROL, flowControl);
    settings->setValue(Constants::SETTINGS_PORTNAME, portName);
    settings->setValue(Constants::SETTINGS_INITIAL_DTR_STATE, initialDtrState);
    settings->setValue(Constants::SETTINGS_INITIAL_RTS_STATE, initialRtsState);
    settings->setValue(Constants::SETTINGS_DEFAULT_LINE_ENDING_INDEX, defaultLineEndingIndex);
    settings->setValue(Constants::SETTINGS_CLEAR_INPUT_ON_SEND, clearInputOnSend);

    saveLineEndings(*settings);

    settings->endGroup();
    settings->sync();

    edited = false;

    qCDebug(log) << "Settings saved.";
}

// Load available settings from a QSettings
void Settings::load(QtcSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup(Constants::SETTINGS_GROUP);

    readSetting(*settings, baudRate, Constants::SETTINGS_BAUDRATE);
    readSetting(*settings, dataBits, Constants::SETTINGS_DATABITS);
    readSetting(*settings, parity, Constants::SETTINGS_PARITY);
    readSetting(*settings, stopBits, Constants::SETTINGS_STOPBITS);
    readSetting(*settings, flowControl, Constants::SETTINGS_FLOWCONTROL);

    readSetting(*settings, portName, Constants::SETTINGS_PORTNAME);
    readSetting(*settings, initialDtrState, Constants::SETTINGS_INITIAL_DTR_STATE);
    readSetting(*settings, initialRtsState, Constants::SETTINGS_INITIAL_RTS_STATE);
    readSetting(*settings, defaultLineEndingIndex, Constants::SETTINGS_DEFAULT_LINE_ENDING_INDEX);

    readSetting(*settings, clearInputOnSend, Constants::SETTINGS_CLEAR_INPUT_ON_SEND);

    loadLineEndings(*settings);

    settings->endGroup();

    edited = false;

    qCDebug(log) << "Settings loaded.";
}

void Settings::setBaudRate(qint32 br)
{
    if (br <= 0)
        return;

    baudRate = br;
    edited = true;
}

void Settings::setPortName(const QString &name)
{
    portName = name;
    edited = true;
}

QByteArray Settings::defaultLineEnding() const
{
    return defaultLineEndingIndex >= (unsigned int)lineEndings.size()
            ? QByteArray()
            : lineEndings.at(defaultLineEndingIndex).second;
}

QString Settings::defaultLineEndingText() const
{
    return defaultLineEndingIndex >= (unsigned int)lineEndings.size()
            ? QString()
            : lineEndings.at(defaultLineEndingIndex).first;
}

void Settings::setDefaultLineEndingIndex(unsigned int index)
{
    if (index >= (unsigned int)lineEndings.size())
        return;

    defaultLineEndingIndex = index;
    edited = true;
}


void Settings::saveLineEndings(QtcSettings &settings)
{
    settings.beginWriteArray(Constants::SETTINGS_LINE_ENDINGS, lineEndings.size());
    int i = 0;
    for (const QPair<QString, QByteArray>& value : std::as_const(lineEndings)) {
        settings.setArrayIndex(i++);
        settings.setValue(Constants::SETTINGS_LINE_ENDING_NAME, value.first);
        settings.setValue(Constants::SETTINGS_LINE_ENDING_VALUE, value.second);
    }
    settings.endArray();
}

void Settings::loadLineEndings(QtcSettings &settings)
{
    const int size = settings.beginReadArray(Constants::SETTINGS_LINE_ENDINGS);
    if (size > 0) // If null, keep default line endings
        lineEndings.clear();

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        lineEndings.append({
                       settings.value(Constants::SETTINGS_LINE_ENDING_NAME).toString(),
                       settings.value(Constants::SETTINGS_LINE_ENDING_VALUE).toByteArray()
                           });
    }
    settings.endArray();
}

} // namespace Internal
} // namespace SerialTerminal
