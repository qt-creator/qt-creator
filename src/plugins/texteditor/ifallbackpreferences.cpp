#include "ifallbackpreferences.h"

#include <utils/settingsutils.h>

#include <QtCore/QSettings>
#include <QtCore/QStringList>

using namespace TextEditor;

namespace TextEditor {
namespace Internal {

class IFallbackPreferencesPrivate
{
public:
    IFallbackPreferencesPrivate()
        : m_currentFallback(0),
          m_readOnly(false)
    {}

    QList<IFallbackPreferences *> m_fallbacks;
    QMap<QString, IFallbackPreferences *> m_idToFallback;
    QMap<IFallbackPreferences *, bool> m_fallbackToEnabled;
    IFallbackPreferences *m_currentFallback;
    QString m_id;
    QString m_displayName;
    bool m_readOnly;
};

}
}

IFallbackPreferences::IFallbackPreferences(
    const QList<IFallbackPreferences *> &fallbacks,
    QObject *parent) :
    QObject(parent),
    d(new Internal::IFallbackPreferencesPrivate)
{
    d->m_fallbacks = fallbacks;
    for (int i = 0; i < fallbacks.count(); i++) {
        IFallbackPreferences *fallback = fallbacks.at(i);
        d->m_idToFallback.insert(fallback->id(), fallback);
    }
}

IFallbackPreferences::~IFallbackPreferences()
{
    delete d;
}

QString IFallbackPreferences::id() const
{
    return d->m_id;
}

void IFallbackPreferences::setId(const QString &name)
{
    d->m_id = name;
}

QString IFallbackPreferences::displayName() const
{
    return d->m_displayName;
}

void IFallbackPreferences::setDisplayName(const QString &name)
{
    d->m_displayName = name;
}

bool IFallbackPreferences::isReadOnly() const
{
    return d->m_readOnly;
}

void IFallbackPreferences::setReadOnly(bool on)
{
    d->m_readOnly = on;
}

bool IFallbackPreferences::isFallbackEnabled(IFallbackPreferences *fallback) const
{
    return d->m_fallbackToEnabled.value(fallback, true);
}

void IFallbackPreferences::setFallbackEnabled(IFallbackPreferences *fallback, bool on)
{
    if (fallback && !d->m_fallbacks.contains(fallback)) {
        // warning
        return;
    }
    d->m_fallbackToEnabled[fallback] = on;
}

IFallbackPreferences *IFallbackPreferences::clone() const
{
    return 0;
}

QVariant IFallbackPreferences::currentValue() const
{
    return currentPreferences()->value();
}

IFallbackPreferences *IFallbackPreferences::currentPreferences() const
{
    IFallbackPreferences *prefs = (IFallbackPreferences *)this;
    while (prefs->currentFallback())
        prefs = prefs->currentFallback();
    return prefs;
}

QList<IFallbackPreferences *> IFallbackPreferences::fallbacks() const
{
    return d->m_fallbacks;
}

IFallbackPreferences *IFallbackPreferences::currentFallback() const
{
    return d->m_currentFallback;
}

void IFallbackPreferences::setCurrentFallback(IFallbackPreferences *fallback)
{
    if (fallback && !d->m_fallbacks.contains(fallback)) {
        // warning
        return;
    }
    if (d->m_currentFallback == fallback)
        return; // nothing changes

    if (d->m_currentFallback) {
        disconnect(d->m_currentFallback, SIGNAL(currentValueChanged(QVariant)),
                   this, SIGNAL(currentValueChanged(QVariant)));
        disconnect(d->m_currentFallback, SIGNAL(currentPreferencesChanged(TextEditor::IFallbackPreferences *)),
                   this, SIGNAL(currentPreferencesChanged(TextEditor::IFallbackPreferences *)));
    }
    d->m_currentFallback = fallback;
    if (d->m_currentFallback) {
        connect(d->m_currentFallback, SIGNAL(currentValueChanged(QVariant)),
                this, SIGNAL(currentValueChanged(QVariant)));
        connect(d->m_currentFallback, SIGNAL(currentPreferencesChanged(TextEditor::IFallbackPreferences *)),
                   this, SIGNAL(currentPreferencesChanged(TextEditor::IFallbackPreferences *)));
    }
    emit currentFallbackChanged(d->m_currentFallback);
    emit currentPreferencesChanged(currentPreferences());
    emit currentValueChanged(currentValue());
}

QString IFallbackPreferences::currentFallbackId() const
{
    if (currentFallback())
        return currentFallback()->id();
    return id(); // or 0?
}

void IFallbackPreferences::setCurrentFallback(const QString &id)
{
    setCurrentFallback(d->m_idToFallback.value(id));
}

void IFallbackPreferences::toSettings(const QString &category, QSettings *s) const
{
    Utils::toSettings(settingsSuffix(), category, s, this);
}

void IFallbackPreferences::fromSettings(const QString &category, const QSettings *s)
{
    Utils::fromSettings(settingsSuffix(), category, s, this);
}


