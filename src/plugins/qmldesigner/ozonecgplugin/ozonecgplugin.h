/* ozonecgplugin.h
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

#include <iwidgetplugin.h>

namespace OzoneCG {
namespace Designer {

class OzoneCGAssetsView;

class OzoneCGPlugin : public QObject, QmlDesigner::IWidgetPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.ozonecg.DesignerPlugin" FILE "ozonecgplugin.json")

    Q_DISABLE_COPY(OzoneCGPlugin)
    Q_INTERFACES(QmlDesigner::IWidgetPlugin)

public:
    OzoneCGPlugin();

    QString metaInfo() const override;
    QString pluginName() const override;

private:
    OzoneCGAssetsView *m_assetsView = nullptr;
};

} // namespace Designer
} // namespace OzoneCG
