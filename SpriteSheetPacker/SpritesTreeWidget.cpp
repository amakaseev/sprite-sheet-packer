#include "SpritesTreeWidget.h"

SpritesTreeWidget::SpritesTreeWidget(QWidget* parent): QTreeWidget(parent) {
    setAcceptDrops(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setIconSize(QSize(24, 24));
    setRootIsDecorated(true);
    header()->setVisible(false);
}

void SpritesTreeWidget::addContent(const QStringList& content, QTreeWidgetItem* parentItem) {
    if (!parentItem) parentItem = invisibleRootItem();
    for (auto str: content) {
        QFileInfo fi(str);
        if (fi.isDir()) {
            QDir dir(fi.absoluteFilePath());
            QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
            item->setText(0, fi.baseName());
            item->setIcon(0, QFileIconProvider().icon(QFileIconProvider::Folder));
            item->setData(0, Qt::UserRole, fi.absoluteFilePath());

            QStringList entryContent;
            QFileInfoList entryInfoList = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
            for (auto entry: entryInfoList) {
                entryContent.push_back(entry.filePath());
            }
            if (entryContent.size()) {
                addContent(entryContent, item);
            }
        } else {
            QStringList nameFilters;
            nameFilters << "png" << "jpg" << "jpeg" << "gif" << "bmp";
            if (nameFilters.indexOf(fi.completeSuffix()) != -1) {
                QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
                item->setText(0, fi.baseName());
                item->setIcon(0, QIcon(fi.absoluteFilePath()));
                item->setData(0, Qt::UserRole, fi.absoluteFilePath());
            }
        }
    }
}

QStringList SpritesTreeWidget::contentList() {
    QStringList fileList;
    for(int i = 0; i < topLevelItemCount(); i++) {
        QTreeWidgetItem* item = topLevelItem(i);
        fileList.push_back(item->data(0, Qt::UserRole).toString());
    }
    return fileList;
}

void SpritesTreeWidget::refresh() {
    auto content = contentList();
    clear();
    addContent(content);
}

void SpritesTreeWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()) {
        if (event->mimeData()->hasUrls()) {
            event->accept();
            return;
        }
    }
    event->ignore();
}

void SpritesTreeWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()) {
        if (event->mimeData()->hasUrls()) {
            event->accept();
            return;
        }
    }
    event->ignore();
}

void SpritesTreeWidget::dropEvent(QDropEvent* event) {
    QStringList filesList;
    for (auto url: event->mimeData()->urls()) {
        QFileInfo fi(url.toLocalFile());
        filesList.push_back(fi.canonicalFilePath());
    }
    addContent(filesList);
    event->accept();

    emit dropComplete();
}
