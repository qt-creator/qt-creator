#ifndef TABPREFERENCES_H
#define TABPREFERENCES_H

#include "ifallbackpreferences.h"
#include "tabsettings.h"

namespace TextEditor {

class TEXTEDITOR_EXPORT TabPreferences : public IFallbackPreferences
{
    Q_OBJECT
public:
    explicit TabPreferences(
        const QList<IFallbackPreferences *> &fallbacks,
        QObject *parentObject = 0);

    virtual QVariant value() const;
    virtual void setValue(const QVariant &);

    TabSettings settings() const;

    // tracks parent hierarchy until currentParentSettings is null
    TabSettings currentSettings() const;

    virtual void toMap(const QString &prefix, QVariantMap *map) const;
    virtual void fromMap(const QString &prefix, const QVariantMap &map);

public slots:
    void setSettings(const TextEditor::TabSettings &tabSettings);

signals:
    void settingsChanged(const TextEditor::TabSettings &);
    void currentSettingsChanged(const TextEditor::TabSettings &);

protected:
    virtual QString settingsSuffix() const;

private slots:
    void slotCurrentValueChanged(const QVariant &);

private:
    TabSettings m_data;
};

} // namespace TextEditor

#endif // TABPREFERENCES_H
