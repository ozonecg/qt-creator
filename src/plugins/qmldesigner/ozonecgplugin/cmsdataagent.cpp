/* cmsdataagent.cpp
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


#include "cmsdataagent.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace OzoneCG {
namespace Designer {

const char BASE_URL[] = "http://127.0.0.1:8000";

CMSDataAgent::CMSDataAgent(QObject *parent)
    : QObject(parent)
{
    connect(&m_networkAccessManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(handleNetworkAccessManagerReply(QNetworkReply*)));
}

void CMSDataAgent::requestListDir(const QString &path)
{
    QUrl url(QString("%1/dir/%2").arg(BASE_URL).arg(path));
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply *reply = m_networkAccessManager.get(request);
    if (reply) {
        reply->setProperty("path", path);
    }
}

void CMSDataAgent::handleNetworkAccessManagerReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
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
    reply->deleteLater();
}

} // namespace Designer
} // namespace OzoneCG
