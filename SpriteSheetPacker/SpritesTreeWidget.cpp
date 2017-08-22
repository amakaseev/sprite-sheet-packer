#include "SpritesTreeWidget.h"

SpritesTreeWidget::SpritesTreeWidget(QWidget* parent): QTreeWidget(parent) {
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setIconSize(QSize(24, 24));
    setRootIsDecorated(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
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

QList< QPair<QString, QString> > SpritesTreeWidget::fileList() {
    QStringList nameFilter;
    nameFilter << "*.png" << "*.jpg" << "*.jpeg" << "*.gif" << "*.bmp";

    QList< QPair<QString, QString> > fileList;
    for(auto pathName: contentList()) {
        QFileInfo fi(pathName);

        if (fi.isDir()) {
            QDir dir(fi.path());
            QDirIterator fileNames(pathName, nameFilter, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while(fileNames.hasNext()){
                fileNames.next();
                fileList.push_back(qMakePair(fileNames.filePath(), dir.relativeFilePath(fileNames.filePath())));
            }
        } else {
            fileList.push_back(qMakePair(pathName, fi.fileName()));
        }
    }

    return fileList;
}

void SpritesTreeWidget::refresh() {
    auto content = contentList();
    clear();
    addContent(content);
}
