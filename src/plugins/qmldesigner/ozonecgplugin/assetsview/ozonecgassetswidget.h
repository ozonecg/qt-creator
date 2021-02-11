/* ozonecgassetswidget.h
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

namespace Ui {
class OzoneCGAssetsWidget;
}

namespace OzoneCG {
namespace Designer {

class OzoneCGAssetsView;
class OzoneCGAssetsWidgetData;


class OzoneCGAssetsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OzoneCGAssetsWidget(OzoneCGAssetsView *view);
    ~OzoneCGAssetsWidget();

private slots:
    void refreshAssets(const QString &rootPath, const QStringList &itemPaths);

    void on_refreshButton_clicked();

    void on_filterLineEdit_textChanged(const QString &arg1);

private:
    void setupAssetCategories();

    OzoneCGAssetsView *m_assetsView;

    Ui::OzoneCGAssetsWidget *ui;
    OzoneCGAssetsWidgetData *d;
};

} // namespace Designer
} // namespace OzoneCG
