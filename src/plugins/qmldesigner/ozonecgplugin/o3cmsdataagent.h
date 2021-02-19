/* o3cmsdataagent.h
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

#include <QObject>
#include <QNetworkAccessManager>

namespace OzoneCG {
namespace Designer {

class CMSUrls
{
public:
    static QUrl listDirUrl(const QString &path);
    static QUrl previewUrl(const QString &path);
    static QUrl assetUrl(const QString &path);
};

class CMSDataAgent : public QObject
{
    Q_OBJECT
public:
    CMSDataAgent(QObject *parent = nullptr);


    void requestListDir(const QString &path);
    void requestPreview(const QString &path);

signals:
    void listDirResponse(const QString &path, const QStringList &items);
    void previewResponse(const QString &path, const QString &mimeType,
                         const QByteArray &data);

private slots:
    void handleNetworkAccessManagerReply(QNetworkReply *);

private:
    QNetworkAccessManager m_networkAccessManager;

};

} // namespace Designer
} // namespace OzoneCG
