// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QTreeWidget;
class QCheckBox;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Core {

class IDocument;

namespace Internal {

class SaveItemsDialog : public QDialog
{
    Q_OBJECT

public:
    SaveItemsDialog(QWidget *parent, const QList<IDocument *> &items);

    void setMessage(const QString &msg);
    void setAlwaysSaveMessage(const QString &msg);
    bool alwaysSaveChecked();
    QList<IDocument *> itemsToSave() const;
    QStringList filesToDiff() const;

private:
    void collectItemsToSave();
    void collectFilesToDiff();
    void discardAll();
    void updateButtons();
    void adjustButtonWidths();

    QLabel *m_msgLabel;
    QTreeWidget *m_treeWidget;
    QCheckBox *m_saveBeforeBuildCheckBox;
    QDialogButtonBox *m_buttonBox;
    QList<IDocument *> m_itemsToSave;
    QStringList m_filesToDiff;
    QPushButton *m_diffButton = nullptr;
};

} // namespace Internal
} // namespace Core
