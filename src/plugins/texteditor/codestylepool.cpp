/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "codestylepool.h"
#include "icodestylepreferencesfactory.h"
#include "icodestylepreferences.h"
#include "tabsettings.h"

#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/persistentsettings.h>

#include <QMap>
#include <QDir>
#include <QDebug>
#include <QFileInfo>

using namespace TextEditor;

static const char codeStyleDataKey[] = "CodeStyleData";
static const char displayNameKey[] = "DisplayName";
static const char codeStyleDocKey[] = "QtCreatorCodeStyle";

namespace TextEditor {
namespace Internal {

class CodeStylePoolPrivate
{
public:
    CodeStylePoolPrivate()
        : m_factory(0)
    {}
    ~CodeStylePoolPrivate();

    QByteArray generateUniqueId(const QByteArray &id) const;

    ICodeStylePreferencesFactory *m_factory;
    QList<ICodeStylePreferences *> m_pool;
    QList<ICodeStylePreferences *> m_builtInPool;
    QList<ICodeStylePreferences *> m_customPool;
    QMap<QByteArray, ICodeStylePreferences *> m_idToCodeStyle;
    QString m_settingsPath;
};

CodeStylePoolPrivate::~CodeStylePoolPrivate()
{
    delete m_factory;
}

QByteArray CodeStylePoolPrivate::generateUniqueId(const QByteArray &id) const
{
    if (!id.isEmpty() && !m_idToCodeStyle.contains(id))
        return id;

    int idx = id.size();
    while (idx > 0) {
        if (!isdigit(id.at(idx - 1)))
            break;
        idx--;
    }

    const QByteArray baseName = id.left(idx);
    QByteArray newName = baseName.isEmpty() ? "codestyle" : baseName;
    int i = 2;
    while (m_idToCodeStyle.contains(newName))
        newName = baseName + QByteArray::number(i++);

    return newName;
}

}
}

static QString customCodeStylesPath()
{
    QString path = Core::ICore::userResourcePath();
    path.append(QLatin1String("/codestyles/"));
    return path;
}

CodeStylePool::CodeStylePool(ICodeStylePreferencesFactory *factory, QObject *parent)
    : QObject(parent),
      d(new Internal::CodeStylePoolPrivate)
{
    d->m_factory = factory;
}

CodeStylePool::~CodeStylePool()
{
    delete d;
}

QString CodeStylePool::settingsDir() const
{
    const QString suffix = d->m_factory ? d->m_factory->languageId().toString() : QLatin1String("default");
    return customCodeStylesPath().append(suffix);
}

Utils::FileName CodeStylePool::settingsPath(const QByteArray &id) const
{
    Utils::FileName path = Utils::FileName::fromString(settingsDir());
    path.appendPath(QString::fromUtf8(id + ".xml"));
    return path;
}

QList<ICodeStylePreferences *> CodeStylePool::codeStyles() const
{
    return d->m_pool;
}

QList<ICodeStylePreferences *> CodeStylePool::builtInCodeStyles() const
{
    return d->m_builtInPool;
}

QList<ICodeStylePreferences *> CodeStylePool::customCodeStyles() const
{
    return d->m_customPool;
}

ICodeStylePreferences *CodeStylePool::cloneCodeStyle(ICodeStylePreferences *originalCodeStyle)
{
    return createCodeStyle(originalCodeStyle->id(), originalCodeStyle->tabSettings(),
                        originalCodeStyle->value(), originalCodeStyle->displayName());
}

ICodeStylePreferences *CodeStylePool::createCodeStyle(const QByteArray &id, const TabSettings &tabSettings,
                  const QVariant &codeStyleData, const QString &displayName)
{
    if (!d->m_factory)
        return 0;

    ICodeStylePreferences *codeStyle = d->m_factory->createCodeStyle();
    codeStyle->setId(id);
    codeStyle->setTabSettings(tabSettings);
    codeStyle->setValue(codeStyleData);
    codeStyle->setDisplayName(displayName);

    addCodeStyle(codeStyle);

    saveCodeStyle(codeStyle);

    return codeStyle;
}

void CodeStylePool::addCodeStyle(ICodeStylePreferences *codeStyle)
{
    const QByteArray newId = d->generateUniqueId(codeStyle->id());
    codeStyle->setId(newId);

    d->m_pool.append(codeStyle);
    if (codeStyle->isReadOnly())
        d->m_builtInPool.append(codeStyle);
    else
        d->m_customPool.append(codeStyle);
    d->m_idToCodeStyle.insert(newId, codeStyle);
    // take ownership
    codeStyle->setParent(this);

    connect(codeStyle, &ICodeStylePreferences::valueChanged,
            this, &CodeStylePool::slotSaveCodeStyle);
    connect(codeStyle, &ICodeStylePreferences::tabSettingsChanged,
            this, &CodeStylePool::slotSaveCodeStyle);
    connect(codeStyle, &ICodeStylePreferences::displayNameChanged,
            this, &CodeStylePool::slotSaveCodeStyle);
    emit codeStyleAdded(codeStyle);
}

void CodeStylePool::removeCodeStyle(ICodeStylePreferences *codeStyle)
{
    const int idx = d->m_customPool.indexOf(codeStyle);
    if (idx < 0)
        return;

    if (codeStyle->isReadOnly())
        return;

    emit codeStyleRemoved(codeStyle);
    d->m_customPool.removeAt(idx);
    d->m_pool.removeOne(codeStyle);
    d->m_idToCodeStyle.remove(codeStyle->id());

    QDir dir(settingsDir());
    dir.remove(settingsPath(codeStyle->id()).fileName());

    delete codeStyle;
}

ICodeStylePreferences *CodeStylePool::codeStyle(const QByteArray &id) const
{
    return d->m_idToCodeStyle.value(id);
}

void CodeStylePool::loadCustomCodeStyles()
{
    QDir dir(settingsDir());
    const QStringList codeStyleFiles = dir.entryList(QStringList() << QLatin1String("*.xml"), QDir::Files);
    for (int i = 0; i < codeStyleFiles.count(); i++) {
        const QString codeStyleFile = codeStyleFiles.at(i);
        // filter out styles which id is the same as one of built-in styles
        if (!d->m_idToCodeStyle.contains(QFileInfo(codeStyleFile).completeBaseName().toUtf8()))
            loadCodeStyle(Utils::FileName::fromString(dir.absoluteFilePath(codeStyleFile)));
    }
}

ICodeStylePreferences *CodeStylePool::importCodeStyle(const Utils::FileName &fileName)
{
    ICodeStylePreferences *codeStyle = loadCodeStyle(fileName);
    if (codeStyle)
        saveCodeStyle(codeStyle);
    return codeStyle;
}

ICodeStylePreferences *CodeStylePool::loadCodeStyle(const Utils::FileName &fileName)
{
    ICodeStylePreferences *codeStyle = 0;
    Utils::PersistentSettingsReader reader;
    reader.load(fileName);
    QVariantMap m = reader.restoreValues();
    if (m.contains(QLatin1String(codeStyleDataKey))) {
        const QByteArray id = fileName.toFileInfo().completeBaseName().toUtf8();
        const QString displayName = reader.restoreValue(QLatin1String(displayNameKey)).toString();
        const QVariantMap map = reader.restoreValue(QLatin1String(codeStyleDataKey)).toMap();
        if (d->m_factory) {
            codeStyle = d->m_factory->createCodeStyle();
            codeStyle->setId(id);
            codeStyle->setDisplayName(displayName);
            codeStyle->fromMap(QString(), map);

            addCodeStyle(codeStyle);
        }
    }
    return codeStyle;
}

void CodeStylePool::slotSaveCodeStyle()
{
    ICodeStylePreferences *codeStyle = qobject_cast<ICodeStylePreferences *>(sender());
    if (!codeStyle)
        return;

    saveCodeStyle(codeStyle);
}

void CodeStylePool::saveCodeStyle(ICodeStylePreferences *codeStyle) const
{
    const QString codeStylesPath = customCodeStylesPath();

    // Create the base directory when it doesn't exist
    if (!QFile::exists(codeStylesPath) && !QDir().mkpath(codeStylesPath)) {
        qWarning() << "Failed to create code style directory:" << codeStylesPath;
        return;
    }
    const QString languageCodeStylesPath = settingsDir();
    // Create the base directory for the language when it doesn't exist
    if (!QFile::exists(languageCodeStylesPath) && !QDir().mkpath(languageCodeStylesPath)) {
        qWarning() << "Failed to create language code style directory:" << languageCodeStylesPath;
        return;
    }

    exportCodeStyle(settingsPath(codeStyle->id()), codeStyle);
}

void CodeStylePool::exportCodeStyle(const Utils::FileName &fileName, ICodeStylePreferences *codeStyle) const
{
    QVariantMap map;
    codeStyle->toMap(QString(), &map);

    QVariantMap tmp;
    tmp.insert(QLatin1String(displayNameKey), codeStyle->displayName());
    tmp.insert(QLatin1String(codeStyleDataKey), map);
    Utils::PersistentSettingsWriter writer(fileName, QLatin1String(codeStyleDocKey));
    writer.save(tmp, Core::ICore::mainWindow());
}

