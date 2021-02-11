/* ozonecgassetsview.cpp
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

#include "ozonecgassetsview.h"
#include "ozonecgassetswidget.h"

namespace OzoneCG {
namespace Designer {

OzoneCGAssetsView::OzoneCGAssetsView()
{
}

bool OzoneCGAssetsView::hasWidget() const
{
    return true;
}

WidgetInfo OzoneCGAssetsView::widgetInfo()
{
    return createWidgetInfo(createWidget(),
                            nullptr,
                            QStringLiteral("OzoneCGAssets"),
                            WidgetInfo::LeftPane,
                            0,
                            tr("OzoneCG Assets"));
}

OzoneCGAssetsWidget *OzoneCGAssetsView::createWidget()
{
    if (!m_widget) {
        m_widget = new OzoneCGAssetsWidget(this);
    }
    return m_widget;
}


} // namespace Designer
} // namespace OzoneCG
