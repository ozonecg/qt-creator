/* o3assetswidget.cpp
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

#include "o3assetswidget.h"
#include "ui_o3assetswidget.h"

#include "o3assetsview.h"
#include "../o3cmsdataagent.h"

#include <itemlibraryinfo.h>

#include <QTreeView>
#include <QDrag>
#include <QMimeData>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QHash>

using namespace QmlDesigner;

namespace OzoneCG {
namespace Designer {

const char FOLDER_ICON_PATH[] = ":/ozonecgplugin/images/folder.png";

const int PATH_ROLE = Qt::UserRole + 1;
const int IS_ASSET_ROLE = Qt::UserRole + 2;


namespace Internal {

class AssetsWidgetPrivate
{
    friend class OzoneCG::Designer::AssetsWidget;

    void refreshAssets(const QString &rootPath, const QStringList &itemPaths);
    QStandardItem *findItem(const QString &path);
    QStandardItem *clearAssetsUnderFolder(const QString &folderPath);
    QStandardItem *insertFolder(const QString &path);
    QStandardItem *insertAsset(const QString &path);

    QStandardItemModel m_assetsModel;
    QSortFilterProxyModel m_proxyModel;
    CMSDataAgent m_cmsDataAgent;

    QHash<QString, QStandardItem*> m_pathItemMap;

    QStringList m_topLevelPaths;
};


static QString parentPath(const QString &path)
{
    auto index = path.lastIndexOf('/');
    if (index > 0)
        return path.mid(0, index);
    return QString();
}

static QString itemName(const QString &path)
{
    auto index = path.lastIndexOf('/');
    if (index >= 0 && (index + 1) < path.size())
        return path.mid(index + 1);
    return path;
}

QStandardItem *AssetsWidgetPrivate::findItem(const QString &path)
{
    QStandardItem *foundItem = m_pathItemMap.value(path, nullptr);
    if (foundItem)
        return foundItem;

    QStandardItem *item = m_assetsModel.invisibleRootItem();
    int index = 0;
    while ((index = path.indexOf('/', index)) > 0) {
        QString partPath = path.mid(0, index);
        QStandardItem *checkItem = m_pathItemMap.value(partPath, nullptr);
        if (checkItem) {
            item = checkItem;
        }
        else {
            for (int row = 0; row < item->rowCount(); row++) {
                QStandardItem *childItem = item->child(row, 0);
                if (!childItem)
                    continue;
                auto childPath = childItem->data(PATH_ROLE).toString();
                m_pathItemMap[childPath] = childItem;
                if (childPath == partPath) {
                    item = childItem;
                    break;
                }
            }
        }
        index++;
    }
    for (int row = 0; row < item->rowCount(); row++) {
        QStandardItem *childItem = item->child(row, 0);
        if (!childItem)
            continue;
        auto childPath = childItem->data(PATH_ROLE).toString();
        m_pathItemMap[childPath] = childItem;
        if (childPath == path) {
            foundItem = childItem;
            break;
        }
    }
    return foundItem;
}

QStandardItem *AssetsWidgetPrivate::insertAsset(const QString &path)
{
    QStandardItem *assetItem = nullptr;
    auto folderItem = insertFolder(parentPath(path));
    if (folderItem) {
        assetItem = new QStandardItem(itemName(path));
        assetItem->setData(path, PATH_ROLE);
        assetItem->setData(true, IS_ASSET_ROLE);
        assetItem->setEditable(false);
        folderItem->appendRow(assetItem);
        m_pathItemMap[path] = assetItem;
    }
    return assetItem;
}

QStandardItem *AssetsWidgetPrivate::insertFolder(const QString &path)
{
    QStandardItem *folderItem = findItem(path);
    if (folderItem)
        return folderItem;
    auto pPath = parentPath(path);
    if (pPath.isEmpty())
        return folderItem;
    QStandardItem *parentItem = insertFolder(pPath);
    if (parentItem) {
        folderItem = new QStandardItem(itemName(path));
        folderItem->setData(path, PATH_ROLE);
        folderItem->setEditable(false);
        folderItem->setIcon(QIcon(FOLDER_ICON_PATH));
        parentItem->appendRow(folderItem);
        m_pathItemMap[path] = folderItem;
    }
    return folderItem;
}

QStandardItem *AssetsWidgetPrivate::clearAssetsUnderFolder(const QString &folderPath)
{
    auto item = findItem(folderPath);
    if (item)
        item->removeRows(0, item->rowCount());
    return item;
}

QMimeData *getAssetMimeData(const QModelIndex &index)
{
    auto assetPath = index.data(PATH_ROLE).toString();
    auto assetUrl = CMSUrls::assetUrl(assetPath);

    ItemLibraryEntry itemLibraryEntry;
    itemLibraryEntry.setType("QtQuick.Image", 2, 0);
    itemLibraryEntry.setName("Image");
    itemLibraryEntry.addProperty(ItemLibraryEntry::Property("source", "QUrl", assetUrl));

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << itemLibraryEntry;

    auto mimeData = new QMimeData();
    mimeData->setData(QStringLiteral("application/vnd.bauhaus.itemlibraryinfo"), data);
    mimeData->removeFormat(QStringLiteral("text/plain"));

    return mimeData;
}

} // namespace Internal


class AssetsTreeView : public QTreeView
{
protected:
    void startDrag(Qt::DropActions supportedActions) override;
};

void AssetsTreeView::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);
    const auto indexes = selectedIndexes();
    if (indexes.isEmpty())
        return;

    const QModelIndex &index = indexes.constFirst();
    if (!index.isValid())
        return;

    auto drag = new QDrag(this);
    drag->setMimeData(Internal::getAssetMimeData(index));
    drag->exec(Qt::CopyAction);
}


AssetsWidget::AssetsWidget(AssetsView *view)
    : m_assetsView(view),
      ui(new Ui::AssetsWidget),
      d(new Internal::AssetsWidgetPrivate)
{
    ui->setupUi(this);
    d->m_proxyModel.setSourceModel(&d->m_assetsModel);
    d->m_proxyModel.setRecursiveFilteringEnabled(true);
    d->m_proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_assetsTreeView = new AssetsTreeView;
    m_assetsTreeView->setDragEnabled(true);
    m_assetsTreeView->setDragDropMode(QAbstractItemView::DragOnly);

    ui->treeViewHostWidget->layout()->addWidget(m_assetsTreeView);
    ui->treeViewHostWidget->layout()->setMargin(0);

    m_assetsTreeView->setModel(&d->m_proxyModel);
    setupAssetCategories();
    connect(&d->m_cmsDataAgent, &CMSDataAgent::listDirResponse,
            this, &AssetsWidget::refreshAssets);
    connect(m_assetsTreeView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &AssetsWidget::handleCurrentRowChanged);
}

AssetsWidget::~AssetsWidget()
{
    delete d;
    delete ui;
}

void AssetsWidget::setupAssetCategories()
{
    QString categoriesData[][3] = {
        {"Components", FOLDER_ICON_PATH, "qml"},
        {"Images", FOLDER_ICON_PATH, "res/images"},
        {"Sequences", FOLDER_ICON_PATH, "res/sequences"},
        {"Videos", FOLDER_ICON_PATH, "res/videos"},
        {"Fonts", FOLDER_ICON_PATH, "res/fonts"},
        {"Data Files", FOLDER_ICON_PATH, "data"},
        {"Scripts", FOLDER_ICON_PATH, "js"},
    };
    d->m_assetsModel.clear();
    d->m_topLevelPaths.clear();
    d->m_assetsModel.setHorizontalHeaderLabels(QStringList() << "Assets");
    for (auto &category : categoriesData) {
        QStandardItem *item = new QStandardItem(QIcon(category[1]), category[0]);
        item->setData(category[2], PATH_ROLE);
        item->setEditable(false);
        d->m_assetsModel.appendRow(item);
        d->m_topLevelPaths.append(category[2]);
    }
}

void AssetsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    on_refreshButton_clicked();
}

void AssetsWidget::on_refreshButton_clicked()
{
    QString selectedPath = "";
    QModelIndexList indexes = m_assetsTreeView->selectionModel()->selectedRows();
    if (indexes.size()) {
        QModelIndex sourceIndex = d->m_proxyModel.mapToSource(indexes.first());
        QStandardItem *item = d->m_assetsModel.itemFromIndex(sourceIndex);
        if (item) {
            if (item->rowCount() == 0 && item->parent())
                item = item->parent();
            selectedPath = item->data(PATH_ROLE).toString();
        }
    }
    if (!selectedPath.isEmpty()) {
        d->m_cmsDataAgent.requestListDir(selectedPath);
    }
    else {
        for (const auto &path : qAsConst(d->m_topLevelPaths))
            d->m_cmsDataAgent.requestListDir(path);
    }
}

void AssetsWidget::refreshAssets(const QString &rootPath, const QStringList &itemPaths)
{
    d->clearAssetsUnderFolder(rootPath);
    d->m_pathItemMap.clear();
    for (const auto &itemPath : itemPaths) {
        QString itemAbsPath = rootPath + "/" + itemPath;
        d->insertAsset(itemAbsPath);
    }
}

void AssetsWidget::on_filterLineEdit_textChanged(const QString &text)
{
    d->m_proxyModel.setFilterFixedString(text);
    if (!text.isEmpty())
        m_assetsTreeView->expandAll();
}

void AssetsWidget::handleCurrentRowChanged(const QModelIndex &index)
{
    QModelIndex sourceIndex = d->m_proxyModel.mapToSource(index);
    QStandardItem *item = d->m_assetsModel.itemFromIndex(sourceIndex);
    if (item && item->data(IS_ASSET_ROLE).toBool())
        emit m_assetsView->assetSelected(item->data(PATH_ROLE).toString());
}


} // namespace Designer
} // namespace OzoneCG

