// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codestylepool.h"
#include "icodestylepreferencesfactory.h"
#include "icodestylepreferences.h"
#include "tabsettings.h"

#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/persistentsettings.h>

#include <QMap>
#include <QDebug>

using namespace Utils;

const char codeStyleDataKey[] = "CodeStyleData";
const char displayNameKey[] = "DisplayName";
const char codeStyleDocKey[] = "QtCreatorCodeStyle";

namespace TextEditor {
namespace Internal {

class CodeStylePoolPrivate
{
public:
    CodeStylePoolPrivate() = default;
    ~CodeStylePoolPrivate();

    QByteArray generateUniqueId(const QByteArray &id) const;

    ICodeStylePreferencesFactory *m_factory = nullptr;
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
    QByteArray newName = baseName.isEmpty() ? QByteArray("codestyle") : baseName;
    int i = 2;
    while (m_idToCodeStyle.contains(newName))
        newName = baseName + QByteArray::number(i++);

    return newName;
}

} // Internal

static FilePath customCodeStylesPath()
{
    return Core::ICore::userResourcePath("codestyles");
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

FilePath CodeStylePool::settingsDir() const
{
    const QString suffix = d->m_factory ? d->m_factory->languageId().toString() : QLatin1String("default");
    return customCodeStylesPath().pathAppended(suffix);
}

FilePath CodeStylePool::settingsPath(const QByteArray &id) const
{
    return settingsDir().pathAppended(QString::fromUtf8(id + ".xml"));
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
        return nullptr;

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

    auto doSaveStyle = [this, codeStyle] { saveCodeStyle(codeStyle); };
    connect(codeStyle, &ICodeStylePreferences::valueChanged, this, doSaveStyle);
    connect(codeStyle, &ICodeStylePreferences::tabSettingsChanged, this, doSaveStyle);
    connect(codeStyle, &ICodeStylePreferences::displayNameChanged, this, doSaveStyle);
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

    settingsPath(codeStyle->id()).removeFile();

    delete codeStyle;
}

ICodeStylePreferences *CodeStylePool::codeStyle(const QByteArray &id) const
{
    return d->m_idToCodeStyle.value(id);
}

void CodeStylePool::loadCustomCodeStyles()
{
    FilePath dir = settingsDir();
    const FilePaths codeStyleFiles = dir.dirEntries({QStringList(QLatin1String("*.xml")), QDir::Files});
    for (const FilePath &codeStyleFile : codeStyleFiles) {
        // filter out styles which id is the same as one of built-in styles
        if (!d->m_idToCodeStyle.contains(codeStyleFile.completeBaseName().toUtf8()))
            loadCodeStyle(codeStyleFile);
    }
}

ICodeStylePreferences *CodeStylePool::importCodeStyle(const FilePath &fileName)
{
    ICodeStylePreferences *codeStyle = loadCodeStyle(fileName);
    if (codeStyle)
        saveCodeStyle(codeStyle);
    return codeStyle;
}

ICodeStylePreferences *CodeStylePool::loadCodeStyle(const FilePath &fileName)
{
    ICodeStylePreferences *codeStyle = nullptr;
    PersistentSettingsReader reader;
    reader.load(fileName);
    Store m = reader.restoreValues();
    if (m.contains(codeStyleDataKey)) {
        const QByteArray id = fileName.completeBaseName().toUtf8();
        const QString displayName = reader.restoreValue(displayNameKey).toString();
        const Store map = storeFromVariant(reader.restoreValue(codeStyleDataKey));
        if (d->m_factory) {
            codeStyle = d->m_factory->createCodeStyle();
            codeStyle->setId(id);
            codeStyle->setDisplayName(displayName);
            codeStyle->fromMap(map);

            addCodeStyle(codeStyle);
        }
    }
    return codeStyle;
}

void CodeStylePool::saveCodeStyle(ICodeStylePreferences *codeStyle) const
{
    const FilePath codeStylesPath = customCodeStylesPath();

    // Create the base directory when it doesn't exist
    if (!codeStylesPath.exists() && !codeStylesPath.createDir()) {
        qWarning() << "Failed to create code style directory:" << codeStylesPath;
        return;
    }
    const FilePath languageCodeStylesPath = settingsDir();
    // Create the base directory for the language when it doesn't exist
    if (!languageCodeStylesPath.exists() && !languageCodeStylesPath.createDir()) {
        qWarning() << "Failed to create language code style directory:" << languageCodeStylesPath;
        return;
    }

    exportCodeStyle(settingsPath(codeStyle->id()), codeStyle);
}

void CodeStylePool::exportCodeStyle(const FilePath &fileName, ICodeStylePreferences *codeStyle) const
{
    const Store map = codeStyle->toMap();
    const Store tmp = {
        {displayNameKey, codeStyle->displayName()},
        {codeStyleDataKey, variantFromStore(map)}
    };
    PersistentSettingsWriter writer(fileName, QLatin1String(codeStyleDocKey));
    writer.save(tmp, Core::ICore::dialogParent());
}

} // TextEditor
