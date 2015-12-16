#include <QtXml>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "PListParser.h"
#include "PListSerializer.h"

#define MAX_RECENT 10

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(QIcon("SpritePacker.icns"));
    //setUnifiedTitleAndToolBarOnMac(true);

    _scene = new QGraphicsScene(this);
    //_scene->setBackgroundBrush(QBrush(QPixmap("://res/background_tran.png")));
    _scene->setBackgroundBrush(QBrush(Qt::darkGray));

    ui->graphicsView->setScene(_scene);

    refreshOpenRecentMenu();

    QSettings settings;
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/state").toByteArray());
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/state", saveState());
    delete ui;
}

void MainWindow::refreshOpenRecentMenu() {
    QSettings settings;
    QStringList openRecentList = settings.value("openRecentList").toStringList();
    ui->menuOpen_recent->clear();
    for (auto file: openRecentList) {
        QAction* recentAction = ui->menuOpen_recent->addAction(file);
        connect(recentAction, SIGNAL(triggered()), this, SLOT(openRecent()));
        if (!QFileInfo(file).exists()) {
            recentAction->setEnabled(false);
        }
    }
}

void MainWindow::refreshAtlas() {
    SpriteAtlas atlas(fileListFromTree(), ui->property_textureBorderSpinBox->value(), ui->property_spriteBorderSpinBox->value(), ui->property_trimCropThesholdSpinBox->value(), 1.f);
    atlas.generate();

    const QImage& atlasImage = atlas.image();

    _scene->clear();
    _scene->addRect(atlasImage.rect(), QPen(Qt::darkRed), QBrush(QPixmap("://res/background_tran.png")));
    QGraphicsPixmapItem* atlasPixmapItem = _scene->addPixmap(QPixmap::fromImage(atlasImage));

    QVector<QRect> rects;
    foreach(QRect rect, rects) {
        _scene->addRect(rect, QPen(Qt::darkGreen));
    }

    _scene->addRect(atlasPixmapItem->boundingRect(), QPen(Qt::darkRed));
    _scene->setSceneRect(atlasPixmapItem->boundingRect());

    //1024x1024x4 (RAM: 18.86MB)
    float ram = (atlasImage.width() * atlasImage.height() * 4) / 1024.f / 1024.f;
    ui->labelAtlasInfo->setText(QString("%1x%2x%3 (RAM: %4MB)").arg(atlasImage.width()).arg(atlasImage.height()).arg(4).arg(ram, 0, 'f', 2));
}

void MainWindow::refreshSpritesTree(const QStringList& fileList) {
    ui->spritesTreeWidget->clear();

    // refresh file path list
    foreach(QString filePath, fileList) {
        QFileInfo fi(filePath);

        if (!fi.exists()) {
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->spritesTreeWidget->invisibleRootItem());
            item->setText(0, fi.baseName());
            item->setData(0, Qt::UserRole, fi.absoluteFilePath());
            item->setTextColor(0, Qt::red);

            continue;
        }

        if (fi.isDir()) {
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->spritesTreeWidget->invisibleRootItem());
            item->setText(0, fi.baseName());
            item->setIcon(0, QFileIconProvider().icon(QFileIconProvider::Folder));
            item->setData(0, Qt::UserRole, fi.absoluteFilePath());

            recursiveRefreshFolder(fi.absoluteFilePath(), item);
        } else {
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->spritesTreeWidget->invisibleRootItem());
            item->setText(0, fi.baseName());
            item->setIcon(0, QIcon(fi.absoluteFilePath()));
            item->setData(0, Qt::UserRole, fi.absoluteFilePath());
        }
    }
}

void MainWindow::recursiveRefreshFolder(const QString& folder, QTreeWidgetItem* parentItem) {
    QDir dir(folder);

    // scan folder(s)
    QFileInfoList entryList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
    foreach(QFileInfo fi, entryList) {
        QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
        item->setText(0, fi.baseName());
        item->setIcon(0, QFileIconProvider().icon(QFileIconProvider::Folder));
        item->setData(0, Qt::UserRole, fi.absoluteFilePath()); // absoluteFilePath

        recursiveRefreshFolder(fi.absoluteFilePath(), item);
    }

    QStringList nameFilter;
    nameFilter << "*.png" << "*.jpg" << "*.jpeg" << "*.gif" << "*.bmp";

    // find files
    entryList = dir.entryInfoList(nameFilter, QDir::Files, QDir::Name);
    foreach(QFileInfo fi, entryList) {
        QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
        item->setText(0, fi.baseName());
        item->setIcon(0, QIcon(fi.absoluteFilePath()));
        item->setData(0, Qt::UserRole, fi.absoluteFilePath());
    }
}

void MainWindow::openSpritePack(const QString& fileName) {
    // add to recent file
    QSettings settings;
    QStringList openRecentList = settings.value("openRecentList").toStringList();
    openRecentList.removeAll(fileName);
    openRecentList.prepend(fileName);
    while (openRecentList.size() > MAX_RECENT)
        openRecentList.removeLast();

    settings.setValue("openRecentList", openRecentList);
    settings.sync();
    refreshOpenRecentMenu();


    // read sprite packer format
    QDir dir(QFileInfo(fileName).absolutePath());
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QVariant plist = PListParser::parsePList(&file);

    if (plist.isValid()) {
        QVariantMap plistDict = plist.toMap();

        // load property
        QVariantMap propertyMap = plistDict["property"].toMap();
        ui->sprites_prefixLineEdit->setText(propertyMap["spritesPrefix"].toString());

        // PACKING
        QVariantMap packingMap = propertyMap["packing"].toMap();
        ui->property_trimCropThesholdSpinBox->setValue(packingMap["threshold"].toInt());
        ui->property_textureBorderSpinBox->setValue(packingMap["textureBorder"].toInt());
        ui->property_spriteBorderSpinBox->setValue(packingMap["spriteBorder"].toInt());

        // OUTPUT
        QVariantMap outputMap = propertyMap["output"].toMap();
        ui->output_dataFormatComboBox->setCurrentIndex(outputMap["dataFormat"].toInt());
        ui->output_destPathLineEdit->setText(QDir(dir.absoluteFilePath(outputMap["destPath"].toString())).absolutePath());
        ui->output_spriteSheetLineEdit->setText(outputMap["spriteSheetName"].toString());
        QVariantMap hdrMap = outputMap["HDR"].toMap();
        ui->output_HDR_groupBox->setChecked(hdrMap["enable"].toBool());
        ui->output_HDR_scaleSpinBox->setValue(hdrMap["scale"].toInt());
        ui->output_HDR_folderLineEdit->setText(hdrMap["folder"].toString());
        QVariantMap hdMap = outputMap["HD"].toMap();
        ui->output_HD_groupBox->setChecked(hdMap["enable"].toBool());
        ui->output_HD_scaleSpinBox->setValue(hdMap["scale"].toInt());
        ui->output_HD_folderLineEdit->setText(hdMap["folder"].toString());
        QVariantMap sdMap = outputMap["SD"].toMap();
        ui->output_SD_groupBox->setChecked(sdMap["enable"].toBool());
        ui->output_SD_scaleSpinBox->setValue(sdMap["scale"].toInt());
        ui->output_SD_folderLineEdit->setText(sdMap["folder"].toString());


        QStringList fileList;
        QVariantList spritesList = plistDict["sprites"].toList();
        foreach (QVariant spriteFile, spritesList) {
            fileList.append(dir.absoluteFilePath(spriteFile.toString()));
        }

        refreshSpritesTree(fileList);
        refreshAtlas();
        on_toolButtonZoomFit_clicked();
    } else {
        QMessageBox::critical(this,
                             "ERROR",
                             "File format error.");
        return;
    }
}

void MainWindow::saveSpritePack(const QString& fileName) {
    // add to recent file
    QSettings settings;
    QVariantList openRecentList = settings.value("openRecentList").toList();
    QStringList openRecentFilesList;
    foreach(QVariant recent, openRecentList) {
        openRecentFilesList.append(recent.toString());
    }
    openRecentFilesList.append(fileName);
    if (openRecentList.count()>10) openRecentList.removeFirst();
    openRecentFilesList.removeDuplicates();
    settings.setValue("openRecentList", openRecentFilesList);
    refreshOpenRecentMenu();

    QDir dir(QFileInfo(fileName).absolutePath());
    QVariantMap plist;

    // save property
    QVariantMap propertyMap;
    propertyMap["spritesPrefix"] = ui->sprites_prefixLineEdit->text();

    // PACKING
    QVariantMap packingMap;
    packingMap["threshold"] = ui->property_trimCropThesholdSpinBox->value();
    packingMap["textureBorder"] = ui->property_textureBorderSpinBox->value();
    packingMap["spriteBorder"] = ui->property_spriteBorderSpinBox->value();
    propertyMap["packing"] = packingMap;

    // OUTPUT
    QVariantMap outputMap;
    outputMap["dataFormat"] = ui->output_dataFormatComboBox->currentIndex();
    outputMap["destPath"] = dir.relativeFilePath(ui->output_destPathLineEdit->text());
    outputMap["spriteSheetName"] = ui->output_spriteSheetLineEdit->text();
    QVariantMap hdrMap;
    hdrMap["enable"] = QVariant(ui->output_HDR_groupBox->isChecked());
    hdrMap["scale"] = QVariant(ui->output_HDR_scaleSpinBox->value());
    hdrMap["folder"] = QVariant(ui->output_HDR_folderLineEdit->text());
    QVariantMap hdMap;
    hdMap["enable"] = QVariant(ui->output_HD_groupBox->isChecked());
    hdMap["scale"] = QVariant(ui->output_HD_scaleSpinBox->value());
    hdMap["folder"] = QVariant(ui->output_HD_folderLineEdit->text());
    QVariantMap sdMap;
    sdMap["enable"] = QVariant(ui->output_SD_groupBox->isChecked());
    sdMap["scale"] = QVariant(ui->output_SD_scaleSpinBox->value());
    sdMap["folder"] = QVariant(ui->output_SD_folderLineEdit->text());
    outputMap["HDR"] = hdrMap;
    outputMap["HD"] = hdMap;
    outputMap["SD"] = sdMap;
    propertyMap["output"] = outputMap;

    plist["property"] = propertyMap;

    // SPRITES
    plist["sprites"] = QVariant(fileListFromTree());


    QFile file(fileName);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(PListSerializer::toPList(plist).toLatin1());
}

void MainWindow::publishSpriteSheet(const QString& fileName, const QString& texName, const QMap<QString, SpriteFrameInfo>& spriteFrames) {
    if (ui->output_dataFormatComboBox->currentIndex() == 0) {
        QVariantMap plist;

        QVariantMap metadataMap;
        metadataMap["format"] = QVariant(2);
        metadataMap["textureFileName"] = texName;
        plist["metadata"] = metadataMap;

        QVariantMap framesMap;
        auto it_f = spriteFrames.begin();
        for (; it_f != spriteFrames.end(); ++it_f) {
            QVariantMap frameMap;
            frameMap["frame"] = QString("{{%1,%2},{%3,%4}}")
                    .arg(it_f.value().mFrame.left())
                    .arg(it_f.value().mFrame.top())
                    .arg(it_f.value().mFrame.width())
                    .arg(it_f.value().mFrame.height());
            frameMap["rotated"] = QVariant(it_f.value().mRotated);
            frameMap["offset"] = QString("{%1,%2}")
                    .arg(it_f.value().mOffset.x())
                    .arg(it_f.value().mOffset.y());
    //        frameMap["sourceColorRect"] = QString("{{%1,%2},{%3,%4}}")
    //                .arg(it_f.value().mSourceColorRect.left())
    //                .arg(it_f.value().mSourceColorRect.top())
    //                .arg(it_f.value().mSourceColorRect.width())
    //                .arg(it_f.value().mSourceColorRect.height());
            frameMap["sourceSize"] = QString("{%1,%2}")
                    .arg(it_f.value().mSourceSize.width())
                    .arg(it_f.value().mSourceSize.height());

            framesMap[ui->sprites_prefixLineEdit->text() + it_f.key()] = frameMap;
        }

        plist["frames"] = framesMap;
        qDebug() << "spriteSheet:" << fileName;

        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        file.write(PListSerializer::toPList(plist).toLatin1());
    } else if (ui->output_dataFormatComboBox->currentIndex() == 1) {
        QString jsonString;
        jsonString += "{";
        auto it_f = spriteFrames.begin();
        for (; it_f != spriteFrames.end(); ++it_f) {
            jsonString += "\"" + QFileInfo(it_f.key()).baseName() + "\":\"";
            jsonString += QString("%1,%2,%3,%4")
                    .arg((int)(it_f.value().mFrame.left()))
                    .arg((int)(it_f.value().mFrame.top()))
                    .arg((int)(it_f.value().mFrame.width()))
                    .arg((int)(it_f.value().mFrame.height()));
            jsonString += "\",";
        }
        jsonString.remove(jsonString.length()-1, 1);
        jsonString += "}";

        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        file.write(jsonString.toLatin1());
    }
}


void MainWindow::on_actionNew_triggered()
{

}

void MainWindow::on_actionOpen_triggered()
{
    QSettings settings;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open sprite packer."),
                                                    settings.value("spritePackerFileName", QDir::currentPath()).toString(),
                                                    tr("Sprite packer (*.sp)"));
    if (!fileName.isEmpty()) {
        settings.setValue("spritePackerFileName", fileName);
        openSpritePack(fileName);
    }
}

void MainWindow::on_actionSave_triggered()
{
    QSettings settings;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save sprite packer."),
                                                    settings.value("spritePackerFileName", QDir::currentPath()).toString(),
                                                    tr("Sprite packer (*.sp)"));
    if (!fileName.isEmpty()) {
        settings.setValue("spritePackerFileName", fileName);

        saveSpritePack(fileName);
    }
}

void MainWindow::openRecent() {
    QAction* senderAction = dynamic_cast<QAction*>(sender());
    openSpritePack(senderAction->text());
}

QStringList MainWindow::fileListFromTree() {
    QStringList fileList;
    for(int i = 0; i < ui->spritesTreeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = ui->spritesTreeWidget->topLevelItem(i);
        fileList.push_back(item->data(0, Qt::UserRole).toString());
    }
    return fileList;
}

void MainWindow::on_actionRefresh_triggered()
{
    refreshSpritesTree(fileListFromTree());
    refreshAtlas();
}

void MainWindow::on_toolButtonZoomOut_clicked()
{
    ui->zoomSlider->setValue(ui->zoomSlider->value() - ui->zoomSlider->singleStep());
}

void MainWindow::on_toolButtonZoomIn_clicked()
{
    ui->zoomSlider->setValue(ui->zoomSlider->value() + ui->zoomSlider->singleStep());
}

void MainWindow::on_toolButtonZoom1x1_clicked()
{
    ui->zoomSlider->setValue(0);
}

void MainWindow::on_toolButtonZoomFit_clicked()
{
    ui->graphicsView->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);
    QTransform tr = ui->graphicsView->transform();
    float scale = tr.m11();
    int value = 50 * scale - 50;
    if (value > 0) {
        value = ((int)(value/5.f))*5;
    } else {
        value = (floor(value/5.f))*5;
    }
    ui->zoomSlider->setValue(value);
}

void MainWindow::on_zoomSlider_valueChanged(int value)
{
    float scale = (value + 50.f) / 50.f;
    ui->graphicsView->setTransform(QTransform::fromScale(scale, scale));

    ui->labelZoomPercent->setText(QString::number((int)(scale * 100)) + " %");
}

void MainWindow::on_toolButtonAddSprites_clicked()
{
    QSettings settings;
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                                          tr("Open Image"),
                                                          settings.value("spritesPath", QDir::currentPath()).toString(),
                                                          tr("Images (*.bmp *.jpg *.jpeg *.gif *.png)"));
    if (fileNames.size()) {
        foreach(const QString& fileName, fileNames) {
            QFileInfo fi(fileName);
            qDebug() << fileName;

            QTreeWidgetItem* item = new QTreeWidgetItem(ui->spritesTreeWidget->invisibleRootItem());
            item->setText(0, fi.baseName());
            item->setIcon(0, QIcon(fi.absoluteFilePath()));
            item->setData(0, Qt::UserRole, fi.absoluteFilePath());
        }
        settings.setValue("spritesPath", fileNames.back());

        refreshSpritesTree(fileListFromTree());
        refreshAtlas();
    }
}

void MainWindow::on_spritesTreeWidget_itemSelectionChanged()
{
    ui->toolButtonRemoveSprites->setEnabled(false);
    foreach (QTreeWidgetItem* item, ui->spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            ui->toolButtonRemoveSprites->setEnabled(true);
            return;
        }
    }
}

void MainWindow::on_toolButtonAddFolder_clicked()
{
    QSettings settings;
    QString pathName = QFileDialog::getExistingDirectory(this,
                                                         tr("Open Folder"),
                                                         settings.value("spritesPath", QDir::currentPath()).toString(),
                                                         QFileDialog::DontResolveSymlinks);
    if (!pathName.isEmpty()) {
        QFileInfo fi(pathName);
        settings.setValue("spritesPath", pathName);
        qDebug() << pathName;

        QTreeWidgetItem* item = new QTreeWidgetItem(ui->spritesTreeWidget->invisibleRootItem());
        item->setText(0, fi.baseName());
        item->setIcon(0, QFileIconProvider().icon(QFileIconProvider::Folder));
        item->setData(0, Qt::UserRole, fi.absoluteFilePath());

        refreshSpritesTree(fileListFromTree());
        refreshAtlas();
    }
}

void MainWindow::on_toolButtonRemoveSprites_clicked()
{
    foreach (QTreeWidgetItem* item, ui->spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            ui->spritesTreeWidget->removeItemWidget(item, 0);
            delete item;
        }
    }

    refreshSpritesTree(fileListFromTree());
    refreshAtlas();
}

void MainWindow::on_output_destFolderToolButton_clicked()
{
    QString destPath = ui->output_destPathLineEdit->text();
    destPath = QFileDialog::getExistingDirectory(this,
                                                 tr("Destination folder"),
                                                 destPath,
                                                 QFileDialog::DontResolveSymlinks);
    if (!destPath.isEmpty()) {
        ui->output_destPathLineEdit->setText(destPath);
    }
}

void MainWindow::on_output_publishPushButton_clicked()
{
    QDir dir(ui->output_destPathLineEdit->text());
    if ((!dir.exists())||(ui->output_destPathLineEdit->text().isEmpty())) {
        QMessageBox::critical(this, "Publish error", "Destination folder invalid!");
        ui->output_destPathLineEdit->setFocus();
        return;
    }

    QString spriteSheetName = ui->output_spriteSheetLineEdit->text();
    if (spriteSheetName.isEmpty()) {
        QMessageBox::critical(this, "Publish error", "Sprite sheet name invalid!");
        ui->output_spriteSheetLineEdit->setFocus();
        return;
    }

    //You must to choose at least one mode
    if ((!ui->output_HDR_groupBox->isChecked())&&(!ui->output_HD_groupBox->isChecked())&&(!ui->output_SD_groupBox->isChecked())){
        QMessageBox::critical(this, "Publish error", "You have to choose at least one mode: HDR/HD/SD");
        return;
    }

    QString imageFileName = ui->output_spriteSheetLineEdit->text() + ".png";
    QString plistFileName;
    switch (ui->output_dataFormatComboBox->currentIndex()) {
        case 0: plistFileName = ui->output_spriteSheetLineEdit->text() + ".plist"; break;
        case 1: plistFileName = ui->output_spriteSheetLineEdit->text() + ".json"; break;
    }

    // HDR
    if (ui->output_HDR_groupBox->isChecked()) {
        dir.mkpath(ui->output_HDR_folderLineEdit->text());
        QDir hdrPath(dir.absoluteFilePath(ui->output_HDR_folderLineEdit->text()));
        QString hdrImageFile = hdrPath.absoluteFilePath(imageFileName);
        QString hdrPlistFile = hdrPath.absoluteFilePath(plistFileName);

        qDebug() << "Publish HDR";
        qDebug() << dir;
        qDebug() << hdrPath;
        qDebug() << hdrImageFile;

        float scale = (float)ui->output_HDR_scaleSpinBox->value() / 100.f;

        SpriteAtlas atlas(fileListFromTree(), ui->property_textureBorderSpinBox->value(), ui->property_spriteBorderSpinBox->value(), ui->property_trimCropThesholdSpinBox->value(), scale);
        atlas.generate();

        const QImage& atlasImage = atlas.image();
        const QMap<QString, SpriteFrameInfo>& spriteFrames = atlas.spriteFrames();

        atlasImage.save(hdrImageFile);
        publishSpriteSheet(hdrPlistFile, imageFileName, spriteFrames);
    }

    // HD
    if (ui->output_HD_groupBox->isChecked()) {
        dir.mkpath(ui->output_HD_folderLineEdit->text());
        QDir hdPath(dir.absoluteFilePath(ui->output_HD_folderLineEdit->text()));
        QString hdImageFile = hdPath.absoluteFilePath(imageFileName);
        QString hdPlistFile = hdPath.absoluteFilePath(plistFileName);

        qDebug() << "Publish HD";
        qDebug() << dir;
        qDebug() << hdPath;
        qDebug() << hdImageFile;

        float scale = (float)ui->output_HD_scaleSpinBox->value() / 100.f;

        SpriteAtlas atlas(fileListFromTree(), ui->property_textureBorderSpinBox->value(), ui->property_spriteBorderSpinBox->value(), ui->property_trimCropThesholdSpinBox->value(), scale);
        atlas.generate();

        const QImage& atlasImage = atlas.image();
        const QMap<QString, SpriteFrameInfo>& spriteFrames = atlas.spriteFrames();

        atlasImage.save(hdImageFile);
        publishSpriteSheet(hdPlistFile, imageFileName, spriteFrames);
    }

    // SD
    if (ui->output_SD_groupBox->isChecked()) {
        dir.mkpath(ui->output_SD_folderLineEdit->text());
        QDir sdPath(dir.absoluteFilePath(ui->output_SD_folderLineEdit->text()));
        QString sdImageFile = sdPath.absoluteFilePath(imageFileName);
        QString sdPlistFile = sdPath.absoluteFilePath(plistFileName);

        qDebug() << "Publish HD";
        qDebug() << dir;
        qDebug() << sdPath;
        qDebug() << sdImageFile;

        float scale = (float)ui->output_SD_scaleSpinBox->value() / 100.f;

        SpriteAtlas atlas(fileListFromTree(), ui->property_textureBorderSpinBox->value(), ui->property_spriteBorderSpinBox->value(), ui->property_trimCropThesholdSpinBox->value(), scale);
        atlas.generate();

        const QImage& atlasImage = atlas.image();
        const QMap<QString, SpriteFrameInfo>& spriteFrames = atlas.spriteFrames();

        atlasImage.save(sdImageFile);
        publishSpriteSheet(sdPlistFile, imageFileName, spriteFrames);
    }
}
