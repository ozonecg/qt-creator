/* o3cmsdataagent.cpp
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


#include "o3cmsdataagent.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace OzoneCG {
namespace Designer {

const char BASE_URL[] = "http://127.0.0.1:8000";

QUrl CMSUrls::listDirUrl(const QString &path)
{
    return QUrl(QString("%1/dir/%2").arg(BASE_URL, path));
}

QUrl CMSUrls::previewUrl(const QString &path)
{
    return QUrl(QString("%1/pvw/%2").arg(BASE_URL, path));
}

QUrl CMSUrls::assetUrl(const QString &path)
{
    return QUrl(QString("%1/%2").arg(BASE_URL, path));
}

CMSDataAgent::CMSDataAgent(QObject *parent)
    : QObject(parent)
{
    connect(&m_networkAccessManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(handleNetworkAccessManagerReply(QNetworkReply*)));
}

void CMSDataAgent::requestListDir(const QString &path)
{
    QNetworkRequest request(CMSUrls::listDirUrl(path));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_networkAccessManager.get(request);
    if (reply) {
        reply->setProperty("path", path);
    }
}

void CMSDataAgent::requestPreview(const QString &path)
{

    QNetworkRequest request(CMSUrls::previewUrl(path));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    m_networkAccessManager.get(request);
}

void CMSDataAgent::handleNetworkAccessManagerReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        auto replyPath = reply->url().path();
        if (replyPath.startsWith("/dir/")) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                QString path = reply->property("path").toString();
                QJsonObject result = doc.object();
                if (result.contains("items")) {
                    QJsonArray itemsArray = result["items"].toArray();
                    QStringList items;
                    for (auto it = itemsArray.begin(); it != itemsArray.end(); ++it) {
                        items.append(it->toString());
                    }
                    emit listDirResponse(path, items);
                }
            }
        }
        else if (replyPath.startsWith("/pvw/")) {
            auto mimeType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
            emit previewResponse(replyPath, mimeType, reply->readAll());
        }
    }
    reply->deleteLater();
}

} // namespace Designer
} // namespace OzoneCG
