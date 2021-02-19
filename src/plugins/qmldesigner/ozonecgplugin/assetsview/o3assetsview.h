/* o3assetsview.h
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

#include <abstractview.h>

namespace OzoneCG {
namespace Designer {

class AssetsWidget;

class AssetsView : public QmlDesigner::AbstractView
{
    Q_OBJECT
public:
    AssetsView();

    bool hasWidget() const override;
    QmlDesigner::WidgetInfo widgetInfo() override;

signals:
    void assetSelected(const QString &path);

private:
    AssetsWidget *createWidget();
    AssetsWidget *m_widget = nullptr;
};

} // namespace Designer
} // namespace OzoneCG
