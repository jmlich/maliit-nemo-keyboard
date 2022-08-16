/*
 * Copyright (C) 2022 Chupligin Sergey <neochapay@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KEYBOARDSLAYOUTMODEL_H
#define KEYBOARDSLAYOUTMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <MGConfItem>
#include <QJsonObject>

class KeyboardsLayoutModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList enabledKeyboards READ enabledKeyboards NOTIFY enabledKeyboardsChanged)
    Q_PROPERTY(QString lastKeyboardLayout READ lastKeyboardLayout WRITE setLastKeyboardLayout NOTIFY lastKeyboardLayoutChanged)

public:
    KeyboardsLayoutModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const {return m_hash;}

    Q_INVOKABLE bool isKeyboardLayoutEnabled(QString code) const;
    Q_INVOKABLE void setKeyboardLayoutEnabled(QString code, bool enabled);
    Q_INVOKABLE QJsonObject getKeyboardByCode(QString code);

    QStringList enabledKeyboards();

    QString lastKeyboardLayout();
    void setLastKeyboardLayout(QString code);

signals:
    void enabledKeyboardsChanged();
    void lastKeyboardLayoutChanged();

private:
    QHash<int,QByteArray> m_hash;
    QStringList m_layoutsFiles;
    QString m_configFilePath;
    const QString m_layoutsDir = "/usr/share/glacier-keyboard/layouts";
    MGConfItem m_enabledLayoutConfigItem;
    MGConfItem m_lastKeyboardLayout;
};

#endif // KEYBOARDSLAYOUTMODEL_H
