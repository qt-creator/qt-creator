#ifndef IFALLBACKPREFERENCES_H
#define IFALLBACKPREFERENCES_H

#include "texteditor_global.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal {
class IFallbackPreferencesPrivate;
}

class TabSettings;

class TEXTEDITOR_EXPORT IFallbackPreferences : public QObject
{
    Q_OBJECT
public:
    explicit IFallbackPreferences(const QList<IFallbackPreferences *> &fallbacks, QObject *parentObject = 0);
    virtual ~IFallbackPreferences();

    QString id() const;
    void setId(const QString &name);

    QString displayName() const;
    void setDisplayName(const QString &name);

    virtual QVariant value() const = 0;
    virtual void setValue(const QVariant &) = 0;

    QVariant currentValue() const; // may be from grandparent

    IFallbackPreferences *currentPreferences() const; // may be grandparent

    QList<IFallbackPreferences *> fallbacks() const;
    IFallbackPreferences *currentFallback() const; // null or one of the above list
    void setCurrentFallback(IFallbackPreferences *fallback);

    QString currentFallbackId() const;
    void setCurrentFallback(const QString &id);

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    // make below 2 protected?
    virtual void toMap(const QString &prefix, QVariantMap *map) const = 0;
    virtual void fromMap(const QString &prefix, const QVariantMap &map) = 0;

signals:
    void valueChanged(const QVariant &);
    void currentValueChanged(const QVariant &);
    void currentFallbackChanged(TextEditor::IFallbackPreferences *currentFallback);

protected:
    virtual QString settingsSuffix() const = 0;

private:
    Internal::IFallbackPreferencesPrivate *d;
};

} // namespace TextEditor

#endif // IFALLBACKPREFERENCES_H
