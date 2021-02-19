/* o3assetspreview.cpp
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

#include "o3assetspreview.h"
#include "o3assetspreviewwidget.h"

using namespace QmlDesigner;

namespace OzoneCG {
namespace Designer {

AssetsPreview::AssetsPreview()
{

}

bool AssetsPreview::hasWidget() const
{
    return true;
}

WidgetInfo AssetsPreview::widgetInfo()
{
    return createWidgetInfo(createWidget(),
                            nullptr,
                            QStringLiteral("OzoneCGAssetsPreview"),
                            WidgetInfo::LeftPane,
                            0,
                            tr("O3 Assets Preview"));
}

AssetsPreviewWidget *AssetsPreview::createWidget()
{
    if (!m_widget) {
        m_widget = new AssetsPreviewWidget(this);
    }
    return m_widget;
}

void AssetsPreview::requestPreview(const QString &path)
{
    m_widget->requestPreview(path);
}


} // namespace Designer
} // namespace OzoneCG
