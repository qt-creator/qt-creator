#ifndef CPPCODESTYLEPREFERENCES_H
#define CPPCODESTYLEPREFERENCES_H

#include "cpptools_global.h"
#include "cppcodestylesettings.h"
#include <texteditor/ifallbackpreferences.h>

namespace CppTools {

class CPPTOOLS_EXPORT CppCodeStylePreferences : public TextEditor::IFallbackPreferences
{
    Q_OBJECT
public:
    explicit CppCodeStylePreferences(
        const QList<TextEditor::IFallbackPreferences *> &fallbacks,
        QObject *parent = 0);

    virtual QVariant value() const;
    virtual void setValue(const QVariant &);

    CppCodeStyleSettings settings() const;

    // tracks parent hierarchy until currentParentSettings is null
    CppCodeStyleSettings currentSettings() const;

    virtual void toMap(const QString &prefix, QVariantMap *map) const;
    virtual void fromMap(const QString &prefix, const QVariantMap &map);

public slots:
    void setSettings(const CppTools::CppCodeStyleSettings &data);

signals:
    void settingsChanged(const CppTools::CppCodeStyleSettings &);
    void currentSettingsChanged(const CppTools::CppCodeStyleSettings &);

protected:
    virtual QString settingsSuffix() const;

private slots:
    void slotCurrentValueChanged(const QVariant &);

private:
    CppCodeStyleSettings m_data;
};

} // namespace CppTools

#endif // CPPCODESTYLEPREFERENCES_H
