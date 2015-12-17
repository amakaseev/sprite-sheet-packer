#include <QtXml>

#include "MainWindow.h"
#include "ScalingVariantWidget.h"
#include "SpritePackerProjectFile.h"
#include "ui_MainWindow.h"

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

    SpritePackerProjectFile::factory().set<SpritePackerProjectFile>("json");
    SpritePackerProjectFile::factory().set<SpritePackerProjectFileOLD>("sp");
    SpritePackerProjectFile::factory().set<SpritePackerProjectFileTPS>("tps");

    _scene = new QGraphicsScene(this);
    //_scene->setBackgroundBrush(QBrush(QPixmap("://res/background_tran.png")));
    _scene->setBackgroundBrush(QBrush(Qt::darkGray));

    ui->graphicsView->setScene(_scene);

    // configure default values
    ui->trimSpinBox->setValue(1);
    ui->textureBorderSpinBox->setValue(0);
    ui->spriteBorderSpinBox->setValue(1);

    on_addScalingVariantPushButton_clicked();


    refreshOpenRecentMenu();

    QSettings settings;
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/state").toByteArray());
}

MainWindow::~MainWindow() {
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

void MainWindow::openRecent() {
    QAction* senderAction = dynamic_cast<QAction*>(sender());
    openSpritePackerProject(senderAction->text());
}

void MainWindow::refreshAtlas() {
    SpriteAtlas atlas(fileListFromTree(), ui->textureBorderSpinBox->value(), ui->spriteBorderSpinBox->value(), ui->trimSpinBox->value(), 1.f);
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

void MainWindow::openSpritePackerProject(const QString& fileName) {
    std::string suffix = QFileInfo(fileName).suffix().toStdString();
    SpritePackerProjectFile* projectFile = SpritePackerProjectFile::factory().get(suffix)();
    if (projectFile) {
        if (!projectFile->read(fileName)) {
            QMessageBox::critical(this, "ERROR", "File format error.");
            return;
        }
    } else {
        QMessageBox::critical(this, "ERROR", "Unknown project file format!");

        return;
    }

    ui->spritesPrefixLineEdit->setText(projectFile->spritesPrefix());
    ui->trimSpinBox->setValue(projectFile->trimThreshold());
    ui->textureBorderSpinBox->setValue(projectFile->textureBorder());
    ui->spriteBorderSpinBox->setValue(projectFile->spriteBorder());
    ui->maxTextureSizeComboBox->setCurrentText(QString::number(projectFile->maxTextureSize()));
    ui->pot2ComboBox->setCurrentIndex(projectFile->pot2()? 0:1);
    switch (projectFile->dataFormat()) {
        case kCocos2D: ui->dataFormatComboBox->setCurrentIndex(0); break;
        case kJson: ui->dataFormatComboBox->setCurrentIndex(1); break;
    }
    ui->destPathLineEdit->setText(projectFile->destPath());
    ui->spriteSheetLineEdit->setText(projectFile->spriteSheetName());

    while(ui->scalingVariantsGroupBox->layout()->count() > 0){
        QLayoutItem *item = ui->scalingVariantsGroupBox->layout()->takeAt(0);
        delete item->widget();
        delete item;
    }
    for (auto scalingVariant: projectFile->scalingVariants()) {
        ScalingVariantWidget* scalingVariantWidget = new ScalingVariantWidget(this, scalingVariant.folderName, scalingVariant.scale);
        connect(scalingVariantWidget, SIGNAL(remove()), this, SLOT(removeScalingVariant()));
        ui->scalingVariantsGroupBox->layout()->addWidget(scalingVariantWidget);

        if (ui->scalingVariantsGroupBox->layout()->count() == 1) {
            scalingVariantWidget->setRemoveEnabled(false);
        }
    }

    refreshSpritesTree(projectFile->srcList());
    refreshAtlas();
    on_toolButtonZoomFit_clicked();


    _currentProjectFileName = fileName;
    setWindowTitle(_currentProjectFileName + " - Sprite Sheet Packer");

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
}

void MainWindow::saveSpritePackerProject(const QString& fileName) {
    std::string suffix = QFileInfo(fileName).suffix().toStdString();
    SpritePackerProjectFile* projectFile = SpritePackerProjectFile::factory().get(suffix)();
    projectFile->setSpritesPrefix(ui->spritesPrefixLineEdit->text());
    projectFile->setTrimThreshold(ui->trimSpinBox->value());
    projectFile->setTextureBorder(ui->textureBorderSpinBox->value());
    projectFile->setSpriteBorder(ui->spriteBorderSpinBox->value());
    projectFile->setMaxTextureSize(ui->maxTextureSizeComboBox->currentText().toInt());
    projectFile->setPot2((ui->pot2ComboBox->currentIndex() == 0)? true:false);
    switch (ui->dataFormatComboBox->currentIndex()) {
        case 0: projectFile->setDataFormat(kCocos2D); break;
        case 1: projectFile->setDataFormat(kJson); break;
    }
    projectFile->setDestPath(ui->destPathLineEdit->text());
    projectFile->setSpriteSheetName(ui->spriteSheetLineEdit->text());

    QVector<ScalingVariant> scalingVariants;
    for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
        if (scalingVariantWidget) {
            ScalingVariant scalingVariant;
            scalingVariant.folderName = scalingVariantWidget->variantFolder();
            scalingVariant.scale = scalingVariantWidget->scale();
            scalingVariants.push_back(scalingVariant);
        }
    }
    projectFile->setScalingVariants(scalingVariants);

    projectFile->setSrcList(fileListFromTree());

    if (projectFile) {
        if (!projectFile->write(fileName)) {
            QMessageBox::critical(this, "ERROR", QString("Not support write to [%1] format.").arg(suffix.c_str()));
            return;
        }
    } else {
        QMessageBox::critical(this, "ERROR", "Unknown project file format!");

        return;
    }

    _currentProjectFileName = fileName;
    setWindowTitle(_currentProjectFileName + " - Sprite Sheet Packer");

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
}

void MainWindow::publishSpriteSheet(const QString& fileName, const QString& texName, const QMap<QString, SpriteFrameInfo>& spriteFrames) {
    if (ui->dataFormatComboBox->currentIndex() == 0) {
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

            framesMap[ui->spritesPrefixLineEdit->text() + it_f.key()] = frameMap;
        }

        plist["frames"] = framesMap;
        qDebug() << "spriteSheet:" << fileName;

        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        file.write(PListSerializer::toPList(plist).toLatin1());
    } else if (ui->dataFormatComboBox->currentIndex() == 1) {
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


void MainWindow::on_actionNew_triggered() {
    ui->spriteSheetLineEdit->setText("");
    ui->spritesTreeWidget->clear();

    _currentProjectFileName.clear();
    setWindowTitle("Sprite Sheet Packer");

    refreshAtlas();
}

void MainWindow::on_actionOpen_triggered() {
    QSettings settings;
    QString dir;
    if (!_currentProjectFileName.isEmpty()) {
        dir = _currentProjectFileName;
    } else {
        dir = settings.value("spritePackerFileName", QDir::currentPath()).toString();
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open sprite packer."),
                                                    dir,
                                                    tr("Sprite sheet packer (*.json *.sp *.tps)"));
    if (!fileName.isEmpty()) {
        settings.setValue("spritePackerFileName", fileName);
        openSpritePackerProject(fileName);
    }
}

void MainWindow::on_actionSave_triggered() {
    if (_currentProjectFileName.isEmpty()) {
        on_actionSaveAs_triggered();
    } else {
        saveSpritePackerProject(_currentProjectFileName);
    }
}

void MainWindow::on_actionSaveAs_triggered() {
    QSettings settings;
    QString dir;
    if (!_currentProjectFileName.isEmpty()) {
        dir = _currentProjectFileName;
    } else {
        dir = settings.value("spritePackerFileName", QDir::currentPath()).toString();
    }
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save sprite packer project file."),
                                                    dir,
                                                    tr("Sprite sheet packer project (*.json)"));
    if (!fileName.isEmpty()) {
        settings.setValue("spritePackerFileName", fileName);
        saveSpritePackerProject(fileName);
    }
}

void MainWindow::on_actionAddSprites_triggered() {
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

void MainWindow::on_actionAddFolder_triggered() {
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

void MainWindow::on_actionRemove_triggered() {
    foreach (QTreeWidgetItem* item, ui->spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            ui->spritesTreeWidget->removeItemWidget(item, 0);
            delete item;
        }
    }

    refreshSpritesTree(fileListFromTree());
    refreshAtlas();
}

void MainWindow::on_actionRefresh_triggered() {
    refreshSpritesTree(fileListFromTree());
    refreshAtlas();
}

void MainWindow::on_actionPublish_triggered() {
    QDir dir(ui->destPathLineEdit->text());
    if ((!dir.exists())||(ui->destPathLineEdit->text().isEmpty())) {
        QMessageBox::critical(this, "Publish error", "Destination folder invalid!");
        ui->destPathLineEdit->setFocus();
        return;
    }

    QString spriteSheetName = ui->spriteSheetLineEdit->text();
    if (spriteSheetName.isEmpty()) {
        QMessageBox::critical(this, "Publish error", "Sprite sheet name invalid!");
        ui->spriteSheetLineEdit->setFocus();
        return;
    }

    QString imageFileName = ui->spriteSheetLineEdit->text() + ".png";
    QString dataFileName;
    switch (ui->dataFormatComboBox->currentIndex()) {
        case 0: dataFileName = ui->spriteSheetLineEdit->text() + ".plist"; break;
        case 1: dataFileName = ui->spriteSheetLineEdit->text() + ".json"; break;
    }

    qDebug() << "Publish to:" << dir;
    for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
        if (scalingVariantWidget) {
            QString folderName = scalingVariantWidget->variantFolder();
            float scale = scalingVariantWidget->scale();

            dir.mkpath(folderName);
            QDir dirPath(dir.absoluteFilePath(folderName));
            QString scaleImageFile = dirPath.absoluteFilePath(imageFileName);
            QString scaleDataFile = dirPath.absoluteFilePath(dataFileName);

            qDebug() << "  folder:" << folderName;
            qDebug() << "  scale:" << scale;

            SpriteAtlas atlas(fileListFromTree(), ui->textureBorderSpinBox->value(), ui->spriteBorderSpinBox->value(), ui->trimSpinBox->value(), scale);
            atlas.generate();

            const QImage& atlasImage = atlas.image();
            const QMap<QString, SpriteFrameInfo>& spriteFrames = atlas.spriteFrames();

            atlasImage.save(scaleImageFile);
            publishSpriteSheet(scaleDataFile, imageFileName, spriteFrames);
        }
    }
}

QStringList MainWindow::fileListFromTree() {
    QStringList fileList;
    for(int i = 0; i < ui->spritesTreeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = ui->spritesTreeWidget->topLevelItem(i);
        fileList.push_back(item->data(0, Qt::UserRole).toString());
    }
    return fileList;
}

void MainWindow::on_toolButtonZoomOut_clicked() {
    ui->zoomSlider->setValue(ui->zoomSlider->value() - ui->zoomSlider->singleStep());
}

void MainWindow::on_toolButtonZoomIn_clicked() {
    ui->zoomSlider->setValue(ui->zoomSlider->value() + ui->zoomSlider->singleStep());
}

void MainWindow::on_toolButtonZoom1x1_clicked() {
    ui->zoomSlider->setValue(0);
}

void MainWindow::on_toolButtonZoomFit_clicked() {
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

void MainWindow::on_zoomSlider_valueChanged(int value) {
    float scale = (value + 50.f) / 50.f;
    ui->graphicsView->setTransform(QTransform::fromScale(scale, scale));

    ui->labelZoomPercent->setText(QString::number((int)(scale * 100)) + " %");
}

void MainWindow::on_spritesTreeWidget_itemSelectionChanged() {
    ui->actionRemove->setEnabled(false);
    foreach (QTreeWidgetItem* item, ui->spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            ui->actionRemove->setEnabled(true);
            return;
        }
    }
}

void MainWindow::on_output_destFolderToolButton_clicked() {
    QString destPath = ui->destPathLineEdit->text();
    destPath = QFileDialog::getExistingDirectory(this,
                                                 tr("Destination folder"),
                                                 destPath,
                                                 QFileDialog::DontResolveSymlinks);
    if (!destPath.isEmpty()) {
        ui->destPathLineEdit->setText(destPath);
    }
}

void MainWindow::on_addScalingVariantPushButton_clicked() {
    ScalingVariantWidget* scalingVariantWidget = new ScalingVariantWidget();
    connect(scalingVariantWidget, SIGNAL(remove()), this, SLOT(removeScalingVariant()));
    ui->scalingVariantsGroupBox->layout()->addWidget(scalingVariantWidget);

    if (ui->scalingVariantsGroupBox->layout()->count() == 1) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(0)->widget());
        if (scalingVariantWidget) {
           scalingVariantWidget->setRemoveEnabled(false);
        }
    }
}

void MainWindow::removeScalingVariant() {
    qDebug() << "remove" << sender();
    ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(sender());
    if (scalingVariantWidget) {
        ui->scalingVariantsGroupBox->layout()->removeWidget(scalingVariantWidget);
        delete scalingVariantWidget;
    }
//    QAction* senderAction = dynamic_cast<QAction*>(sender());
}
