/* o3assetspreviewwidget.cpp
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

#include "o3assetspreviewwidget.h"
#include "ui_o3assetspreviewwidget.h"

#include "../o3cmsdataagent.h"

#include <QPixmap>
#include <QImage>

#include <QVideoWidget>
#include <QMediaPlayer>

#include <QDebug>

namespace OzoneCG {
namespace Designer {

const char BASE_URL[] = "http://127.0.0.1:8000";

namespace Internal {
class AssetsPreviewWidgetData
{
    friend class OzoneCG::Designer::AssetsPreviewWidget;

    QMediaPlayer m_player;
    CMSDataAgent m_cmsDataAgent;
};
}

AssetsPreviewWidget::AssetsPreviewWidget(AssetsPreview *preview) :
    m_assetsPreview(preview),
    ui(new Ui::AssetsPreviewWidget),
    d(new Internal::AssetsPreviewWidgetData)
{
    ui->setupUi(this);
    d->m_player.setVideoOutput(ui->videoWidget);
    connect(&d->m_cmsDataAgent, &CMSDataAgent::previewResponse,
            this, &AssetsPreviewWidget::showPreview);
}

AssetsPreviewWidget::~AssetsPreviewWidget()
{
    delete ui;
}

void AssetsPreviewWidget::requestPreview(const QString &path)
{
    qDebug() << "OzoneCGAssetsPreviewWidget::requestPreview: path: " << path;
    if (path.endsWith(".mp4", Qt::CaseInsensitive)) {
        d->m_player.setMedia(CMSUrls::previewUrl(path));
        d->m_player.play();
        ui->stackedWidget->setCurrentIndex(1);
    }
    else {
        d->m_cmsDataAgent.requestPreview(path);
    }
}

void AssetsPreviewWidget::showPreview(const QString &path,
                                              const QString &mimeType,
                                              const QByteArray &data)
{
    if (mimeType.startsWith("image/")) {
        QPixmap pixmap;
        if (pixmap.loadFromData(data)) {
            ui->imageLabel->setPixmap(pixmap);
        }
        ui->stackedWidget->setCurrentIndex(0);
        d->m_player.stop();
    }
    else if (mimeType.startsWith("video/")) {

        // ui->stackedWidget->setCurrentIndex(1);
    }
}

} // namespace Designer
} // namespace OzoneCG
