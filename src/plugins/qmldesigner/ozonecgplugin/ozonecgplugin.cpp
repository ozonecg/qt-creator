/* ozonecgplugin.cpp
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

#include "ozonecgplugin.h"
#include "assetsview/ozonecgassetsview.h"
#include <qmldesignerplugin.h>
#include <designeractionmanager.h>
#include <modelnodecontextmenu_helper.h>
#include <viewmanager.h>

#include <QDebug>

namespace OzoneCG {
namespace Designer {

using namespace QmlDesigner;

OzoneCGPlugin::OzoneCGPlugin()
    : m_assetsView(new OzoneCGAssetsView)
{
    ViewManager &viewManager = QmlDesignerPlugin::instance()->viewManager();
    viewManager.registerViewTakingOwnership(m_assetsView);
}

QString OzoneCGPlugin::metaInfo() const
{
    return QLatin1String(":/ozonecgplugin/ozonecgplugin.metainfo");
}

QString OzoneCGPlugin::pluginName() const
{
    return QLatin1String("OzoneCGPlugin");
}

} // namespace Designer
} // namespace OzoneCG
