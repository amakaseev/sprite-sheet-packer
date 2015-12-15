#include "binpack2d.hpp"

#include <QtXml>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "imagerotate.h"

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

    _graphicsView = new QGraphicsView(this);
    _graphicsView->setScene(_scene);
    setCentralWidget(_graphicsView);

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

class MyContent {
public:
    MyContent(const QString& fname, const QString& name, const QImage& image) {
        mFileName = fname;
        mName = name;
        mImage = image;
        mRect = QRect(0, 0, mImage.width(), mImage.height());
    }

    void trim(int alpha) {
        int l = mImage.width();
        int t = mImage.height();
        int r = 0;
        int b = 0;
        for (int y=0; y<mImage.height(); y++) {
            bool rowFilled = false;
            for (int x=0; x<mImage.width(); x++) {
                int a = qAlpha(mImage.pixel(x, y));
                if (a >= alpha) {
                    rowFilled = true;
                    r = qMax(r, x);
                    if (l > x) {
                        l = x;
                    }
                }
            }
            if (rowFilled) {
                t = qMin(t, y);
                b = y;
            }
        }
        mRect = QRect(QPoint(l, t), QPoint(r,b));
        if ((mRect.width() % 2) != (mImage.width() % 2)) {
            if (l>0) l--; else r++;
            mRect = QRect(QPoint(l, t), QPoint(r,b));
        }
        if ((mRect.height() % 2) != (mImage.height() % 2)) {
            if (t>0) t--; else b++;
            mRect = QRect(QPoint(l, t), QPoint(r,b));
        }
        if ((mRect.width()<0)||(mRect.height()<0)) {
            mRect = QRect(0, 0, 2, 2);
        }
    }

    QString mFileName;
    QString mName;
    QImage  mImage;
    QRect   mRect;
};

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

void MainWindow::generateAtlas(float scale, QImage& atlasImage, QMap<QString, SpriteFrameInfo>& spriteFrames) {
    BinPack2D::ContentAccumulator<MyContent> inputContent;

    QStringList nameFilter;
    nameFilter << "*.png" << "*.jpg" << "*.jpeg" << "*.gif" << "*.bmp";

    QList< QPair<QString,QString> > fileList;
    for(int i = 0; i < ui->spritesTreeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = ui->spritesTreeWidget->topLevelItem(i);
        QString pathName = item->data(0, Qt::UserRole).toString();
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

    int volume = 0;
    int spriteBorder = ui->property_spriteBorderSpinBox->value();
    int textureBorder = ui->property_textureBorderSpinBox->value();

    // init images and rects
    QList< QPair<QString,QString> >::iterator it_f = fileList.begin();
    for(; it_f != fileList.end(); ++it_f) {
        QImage image((*it_f).first);
        if (image.isNull()) continue;
        if (scale != 1) {
            image = image.scaled(ceil(image.width() * scale), ceil(image.height() * scale), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        MyContent mycontent((*it_f).first, (*it_f).second, image);

        // Trim / Crop
        if (ui->property_trimCropGroupBox->isChecked() && ui->property_trimCropThesholdSpinBox->value()) {
            mycontent.trim(ui->property_trimCropThesholdSpinBox->value());
        }

        int width = mycontent.mRect.width();
        int height = mycontent.mRect.height();
        volume += width * height * 1.02f;

        inputContent += BinPack2D::Content<MyContent>(mycontent,
                                                      BinPack2D::Coord(),
                                                      BinPack2D::Size(width+spriteBorder, height+spriteBorder),
                                                      false);
    }

    // Sort the input content by size... usually packs better.
    inputContent.Sort();

    // A place to store packed content.
    BinPack2D::ContentAccumulator<MyContent> remainder;
    BinPack2D::ContentAccumulator<MyContent> outputContent;

    // find optimal size for atlas
    int w = sqrt(volume) + textureBorder*2;
    int h = sqrt(volume) + textureBorder*2;
    qDebug() << w << "x" << h;
    bool k = true;
    int step = (w + h) / 20;
    while (1) {
        BinPack2D::CanvasArray<MyContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<MyContent>(w - textureBorder*2, h - textureBorder*2, 1).Build();

        bool success = canvasArray.Place(inputContent, remainder);
        if (success) {
            outputContent = BinPack2D::ContentAccumulator<MyContent>();
            canvasArray.CollectContent(outputContent);
            break;
        }
        if (k) {
            k = false;
            w += step;
        } else {
            k = true;
            h += step;
        }
        qDebug() << "stage 1:" << w << "x" << h << "step:" << step;
    }
    step = (w + h) / 20;
    while (1) {
        w -= step;
        BinPack2D::CanvasArray<MyContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<MyContent>(w - textureBorder*2, h - textureBorder*2, 1).Build();

        bool success = canvasArray.Place(inputContent, remainder);
        if (!success) {
            w += step;
            if (step > 1) step = qMax(step/2, 1); else break;
        } else {
            outputContent = BinPack2D::ContentAccumulator<MyContent>();
            canvasArray.CollectContent(outputContent);
        }
        qDebug() << "stage 2:" << w << "x" << h << "step:" << step;
    }
    step = (w + h) / 20;
    while (1) {
        h -= step;
        BinPack2D::CanvasArray<MyContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<MyContent>(w - textureBorder*2, h - textureBorder*2, 1).Build();

        bool success = canvasArray.Place(inputContent, remainder);
        if (!success) {
            h += step;
            if (step > 1) step = qMax(step/2, 1); else break;
        } else {
            outputContent = BinPack2D::ContentAccumulator<MyContent>();
            canvasArray.CollectContent(outputContent);
        }
        qDebug() << "stage 3:" << w << "x" << h << "step:" << step;
    }


    // parse output.
    typedef BinPack2D::Content<MyContent>::Vector::iterator binpack2d_iterator;

    atlasImage = QImage(w, h, QImage::Format_RGBA8888);
    atlasImage.fill(QColor(0, 0, 0, 0));
    QPainter painter(&atlasImage);

    spriteFrames.clear();
    for( binpack2d_iterator itor = outputContent.Get().begin(); itor != outputContent.Get().end(); itor++ ) {
        const BinPack2D::Content<MyContent> &content = *itor;

        // retreive your data.
        const MyContent &myContent = content.content;
        //qDebug() << myContent.mName << myContent.mRect;

        // image
        QImage image;
        if (content.rotated) {
            image = myContent.mImage.copy(myContent.mRect);
            image = rotate90(image);
        }

        SpriteFrameInfo spriteFrame;
        spriteFrame.mFrame = QRect(content.coord.x + textureBorder, content.coord.y + textureBorder, content.size.w-spriteBorder, content.size.h-spriteBorder);
        spriteFrame.mOffset = QPoint(
                    (myContent.mRect.left() + (-myContent.mImage.width() + content.size.w - spriteBorder) * 0.5f),
                    (-myContent.mRect.top() + ( myContent.mImage.height() - content.size.h + spriteBorder) * 0.5f)
                    );
        spriteFrame.mRotated = content.rotated;
        spriteFrame.mSourceColorRect = myContent.mRect;
        spriteFrame.mSourceSize = myContent.mImage.size();
        if (content.rotated) {
            spriteFrame.mFrame = QRect(content.coord.x, content.coord.y, content.size.h-spriteBorder, content.size.w-spriteBorder);

        }
        spriteFrames[myContent.mName] = spriteFrame;

        if (content.rotated) {
            painter.drawImage(QPoint(content.coord.x+textureBorder, content.coord.y+textureBorder), image);
        } else {
            painter.drawImage(QPoint(content.coord.x+textureBorder, content.coord.y+textureBorder), myContent.mImage, myContent.mRect);
        }
    }
    painter.end();
}


void MainWindow::refreshAtlas() {
    QImage atlasImage;
    QMap<QString, SpriteFrameInfo> spriteFrames;

    generateAtlas(1.f, atlasImage, spriteFrames);

    _scene->clear();
    _scene->addRect(atlasImage.rect(), QPen(Qt::darkRed), QBrush(QPixmap("://res/background_tran.png")));
    QGraphicsPixmapItem* atlasPixmapItem = _scene->addPixmap(QPixmap::fromImage(atlasImage));

    QVector<QRect> rects;
    foreach(QRect rect, rects) {
        _scene->addRect(rect, QPen(Qt::darkGreen));
    }

    _scene->addRect(atlasPixmapItem->boundingRect(), QPen(Qt::darkRed));
    _scene->setSceneRect(atlasPixmapItem->boundingRect());

    ui->statusBar->showMessage(QString("%1x%2").arg(atlasImage.width()).arg(atlasImage.height()));
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
    qDebug() << "scan folder: " << folder;

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
        ui->property_trimCropGroupBox->setChecked(packingMap["trim"].toBool());
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
    packingMap["trim"] = ui->property_trimCropGroupBox->isChecked();
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
    // collect all root folders ans files
    QVariantList spritesList;
    for(int i = 0; i < ui->spritesTreeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = ui->spritesTreeWidget->topLevelItem(i);
        QString pathName = item->data(0, Qt::UserRole).toString();

        spritesList.append(dir.relativeFilePath(QFileInfo(pathName).absoluteFilePath()));
    }
    plist["sprites"] = QVariant(spritesList);


    QFile file(fileName);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(PListSerializer::toPList(plist).toLatin1());
}

void MainWindow::publishSpriteSheet(const QString& fileName, const QString& texName, QMap<QString, SpriteFrameInfo>& spriteFrames) {
    if (ui->output_dataFormatComboBox->currentIndex() == 0) {
        QVariantMap plist;

        QVariantMap metadataMap;
        metadataMap["format"] = QVariant(2);
        metadataMap["textureFileName"] = texName;
        plist["metadata"] = metadataMap;

        QVariantMap framesMap;
        QMap<QString, SpriteFrameInfo>::iterator it_f = spriteFrames.begin();
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
        QMap<QString, SpriteFrameInfo>::iterator it_f = spriteFrames.begin();
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

void MainWindow::on_actionAddSprites_triggered()
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

        refreshAtlas();
    }
}

void MainWindow::on_actionAddFolder_triggered()
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

        recursiveRefreshFolder(fi.absoluteFilePath(), item);

        refreshAtlas();
    }
}

void MainWindow::on_actionRemove_triggered()
{
    if (ui->spritesTreeWidget->selectedItems().size() < 1) return;
    if (QMessageBox::question(this, "Remove sprites", "You really want to remove the sprites?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        foreach (QTreeWidgetItem* item, ui->spritesTreeWidget->selectedItems()) {
            ui->spritesTreeWidget->removeItemWidget(item, 0);
            delete item;
        }

        refreshAtlas();
    }
}

void MainWindow::on_actionRefresh_triggered()
{
    QStringList fileList;
    for(int i = 0; i < ui->spritesTreeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = ui->spritesTreeWidget->topLevelItem(i);
        fileList.push_back(item->data(0, Qt::UserRole).toString());
    }

    refreshSpritesTree(fileList);
    refreshAtlas();
}

void MainWindow::on_actionZoomIn_triggered()
{
    _graphicsView->scale(2.f, 2.f);
    //_graphicsView->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::on_actionZoomOut_triggered()
{
    _graphicsView->scale(0.5f, 0.5f);
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

        QImage atlasImage;
        QMap<QString, SpriteFrameInfo> spriteFrames;

        float scale = (float)ui->output_HDR_scaleSpinBox->value() / 100.f;
        generateAtlas(scale, atlasImage, spriteFrames);

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

        QImage atlasImage;
        QMap<QString, SpriteFrameInfo> spriteFrames;

        float scale = (float)ui->output_HD_scaleSpinBox->value() / 100.f;
        generateAtlas(scale, atlasImage, spriteFrames);

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

        QImage atlasImage;
        QMap<QString, SpriteFrameInfo> spriteFrames;

        float scale = (float)ui->output_SD_scaleSpinBox->value() / 100.f;
        generateAtlas(scale, atlasImage, spriteFrames);

        atlasImage.save(sdImageFile);
        publishSpriteSheet(sdPlistFile, imageFileName, spriteFrames);
    }

}
