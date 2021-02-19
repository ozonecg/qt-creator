/* o3assetspreviewwidget.h
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
class AssetsPreviewWidget;
}

namespace OzoneCG {
namespace Designer {

class AssetsPreview;

namespace Internal {
class AssetsPreviewWidgetData;
}

class AssetsPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AssetsPreviewWidget(AssetsPreview *preview);
    ~AssetsPreviewWidget();

public slots:
    void requestPreview(const QString &path);
    void showPreview(const QString &path, const QString &mimeType,
                     const QByteArray &data);

private:
    AssetsPreview *m_assetsPreview;

    Ui::AssetsPreviewWidget *ui;
    Internal::AssetsPreviewWidgetData *d;
};

} // namespace Designer
} // namespace OzoneCG
