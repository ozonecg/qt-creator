/* o3assetswidget.h
 *
 * Copyright (C) 2021 Siddharudh P T <siddharudh@gmail.com>
 *
 * This file is part of OzoneCG Project.
 *
 * OzoneCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OzoneCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OzoneCG.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QWidget>
#include <QModelIndex>

namespace Ui {
class AssetsWidget;
}

namespace OzoneCG {
namespace Designer {

class AssetsView;
class AssetsTreeView;

namespace Internal {
class AssetsWidgetPrivate;
}

class AssetsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AssetsWidget(AssetsView *view);
    ~AssetsWidget();

private slots:
    void refreshAssets(const QString &rootPath, const QStringList &itemPaths);

    void on_refreshButton_clicked();

    void on_filterLineEdit_textChanged(const QString &text);

    void handleCurrentRowChanged(const QModelIndex &index);

protected:
    void showEvent(QShowEvent *event) override;


private:
    void setupAssetCategories();

    AssetsView *m_assetsView;

    Ui::AssetsWidget *ui;
    AssetsTreeView *m_assetsTreeView;
    Internal::AssetsWidgetPrivate *d;
};

} // namespace Designer
} // namespace OzoneCG
