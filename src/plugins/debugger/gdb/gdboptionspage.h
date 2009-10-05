#ifndef GDBOPTIONSPAGE_H
#define GDBOPTIONSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/savedaction.h>

#include "ui_gdboptionspage.h"

namespace Debugger {
namespace Internal {

class GdbOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    GdbOptionsPage();

    virtual QString id() const { return settingsId(); }
    virtual QString trName() const;
    virtual QString category() const;
    virtual QString trCategory() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();

    static QString settingsId();

private:
    Ui::GdbOptionsPage m_ui;
    Utils::SavedActionSet m_group;
};

} // namespace Internal
} // namespace Debugger

#endif // GDBOPTIONSPAGE_H
