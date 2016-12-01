#include <QtXml>

#include "MainWindow.h"
#include "SpriteAtlasPreview.h"
#include "PublishSpriteSheet.h"
#include "ScalingVariantWidget.h"
#include "AboutDialog.h"
#include "PreferencesDialog.h"
#include "PublishStatusDialog.h"
#include "ui_MainWindow.h"

#include "PListParser.h"
#include "PListSerializer.h"

#define MAX_RECENT 10

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

#if defined(Q_OS_WIN)
    setWindowIcon(QIcon("SpritePacker.ico"));
#elif defined(Q_OS_MAC)
    setWindowIcon(QIcon("SpritePacker.icns"));
#endif

    setUnifiedTitleAndToolBarOnMac(true);

    _blockUISignals = false;
    _projectDirty = false;

    _spritesTreeWidget = new SpritesTreeWidget(ui->spritesDockWidgetContents);
    connect(_spritesTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(spritesTreeWidgetItemSelectionChanged()));
    ui->spritesDockWidgetLayout->addWidget(_spritesTreeWidget);

    // init image and pixel formats
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kARGB8888));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kARGB8565));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kARGB4444));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kRGB888));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kRGB565));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kALPHA));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kETC1));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kPVRTC2));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kPVRTC2A));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kPVRTC4));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kPVRTC4A));
    ui->pixelFormatComboBox->setCurrentIndex(kARGB8888);
    ui->imageFormatComboBox->insertItem(kPNG, "PNG (.png)");
    ui->imageFormatComboBox->insertItem(kPKM, "PKM with ETC1 (.pkm)");
    ui->imageFormatComboBox->insertItem(kPVR, "PVR (.pvr)");
    ui->imageFormatComboBox->insertItem(kPVR_CCZ, "PVR+zlib (.pvr.ccz)");
    ui->imageFormatComboBox->setCurrentIndex(kPNG);

    // configure default values
    ui->trimSpinBox->setValue(1);
    ui->textureBorderSpinBox->setValue(0);
    ui->spriteBorderSpinBox->setValue(2);

    on_addScalingVariantPushButton_clicked();

    // setup open button
    _openButton = new QToolButton(this);
    _openButton->setIcon(ui->actionOpen->icon());
    _openButton->setText(ui->actionOpen->text());
    _openButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    connect(_openButton, SIGNAL(pressed()), this, SLOT(on_actionOpen_triggered()));
    ui->mainToolBar->insertWidget(ui->actionSave, _openButton);

    ui->optLevelSlider->setVisible(false);
    ui->optLevelText->setVisible(false);
    ui->optLevelLabel->setVisible(false);

    // layout preferences action on right side in toolbar
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->mainToolBar->insertWidget(ui->actionPreferences ,empty);

    createRefreshButton();
    setAcceptDrops(true);

    refreshFormats();
    refreshOpenRecentMenu();

    // reset dirty
    _projectDirty = false;
    _atlasDirty = false;

    QSettings settings;
    
//    ui->displayOutlinesCheckBox->setChecked(settings.value("MainWindow/displayOutlines").toBool());
    
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/state").toByteArray());
}

MainWindow::~MainWindow() {
    QSettings settings;
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/state", saveState());
    delete ui;
}

void MainWindow::refreshFormats() {
    QSettings settings;
    QStringList formatsFolder;
    formatsFolder.push_back(QCoreApplication::applicationDirPath() + "/defaultFormats");
    formatsFolder.push_back(settings.value("Preferences/customFormatFolder").toString());

    // load formats
    PublishSpriteSheet::formats().clear();
    for (auto folder: formatsFolder) {
        if (QDir(folder).exists()) {
            QDirIterator fileNames(folder, QStringList() << "*.js", QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
            while(fileNames.hasNext()) {
                fileNames.next();
                PublishSpriteSheet::addFormat(fileNames.fileInfo().baseName(), fileNames.filePath());
            }
        }
    }

    _blockUISignals = true;
    QString prevFormat = ui->dataFormatComboBox->currentText();
    ui->dataFormatComboBox->clear();
    for (auto format: PublishSpriteSheet::formats().keys()) {
        QString fileName(PublishSpriteSheet::formats()[format]);
        ui->dataFormatComboBox->addItem(QIcon(fileName.left(fileName.lastIndexOf(".")) + ".png"), format);
    }
    ui->dataFormatComboBox->setCurrentText(prevFormat);
    _blockUISignals = false;
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

    if (!openRecentList.empty()) {
        _openButton->setPopupMode(QToolButton::MenuButtonPopup);
        _openButton->setMenu(ui->menuOpen_recent);
    } else {
        _openButton->setMenu(NULL);
    }
}

void MainWindow::openRecent() {
    QAction* senderAction = dynamic_cast<QAction*>(sender());
    openSpritePackerProject(senderAction->text());
}

void MainWindow::createRefreshButton() {
    QSettings settings;
    //setup enable/disable auto refresh atlas checkbox
    auto refreshFrame = new QFrame(this);
    auto vLayout = new QVBoxLayout(refreshFrame);
    // button
    auto refreshButton = new QToolButton(this);
    refreshButton->setAutoRaise(true);
    refreshButton->setIconSize(QSize(24, 24));
    refreshButton->setIcon(QIcon(":/res/refresh-32.png"));
    connect(refreshButton, &QToolButton::pressed, [this](){
        refreshAtlas();
    });
    // checkbox
    auto autoRefreshCheckbox = new QCheckBox("Auto refresh", this);
    auto font = autoRefreshCheckbox->font();
    font.setPointSize(8);
    autoRefreshCheckbox->setFont(font);
    autoRefreshCheckbox->setChecked(settings.value("MainWindow/automaticPreview", true).toBool());
    connect(autoRefreshCheckbox, &QCheckBox::toggled, [](bool value) {
        QSettings settings;
        settings.setValue("MainWindow/automaticPreview", value);
    });
    // layout
    vLayout->addWidget(refreshButton, 0, Qt::AlignHCenter);
    vLayout->addWidget(autoRefreshCheckbox, 0, Qt::AlignHCenter);
    ui->mainToolBar->insertWidget(ui->actionPublish, refreshFrame);
}

void MainWindow::refreshAtlas(bool generate) {
    if (generate) {
        _spriteAtlas.clear();
        for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
            ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
            if (scalingVariantWidget) {
                float scale = scalingVariantWidget->scale();
                int maxTextureSize = scalingVariantWidget->maxTextureSize();
                bool pow2 = scalingVariantWidget->pow2();

                SpriteAtlas atlas = SpriteAtlas(_spritesTreeWidget->contentList(),
                                                ui->textureBorderSpinBox->value(),
                                                ui->spriteBorderSpinBox->value(),
                                                ui->trimSpinBox->value(),
                                                pow2,
                                                maxTextureSize,
                                                scale);

                atlas.setAlgorithm(ui->algorithmComboBox->currentText());

                if (ui->trimModeComboBox->currentText() == "Polygon") {
                    atlas.enablePolygonMode(true, ui->epsilonHorizontalSlider->value() / 10.f);
                }

                if (!atlas.generate()) {
                    QMessageBox::critical(this, "Generate error of scalingVariant:" + scalingVariantWidget->name(), "Max texture size limit is small!");
                } else {
                    _spriteAtlas.push_back(atlas);
                }
            }
        }
    }

    refreshPreview();

    _atlasDirty = false;
}

void MainWindow::refreshPreview() {
    // removing if needed
    while(ui->atlasPreviewTabWidget->count() > _spriteAtlas.size()) {
        ui->atlasPreviewTabWidget->removeTab(ui->atlasPreviewTabWidget->count() - 1);
    }

    // adding if needed
    while(ui->atlasPreviewTabWidget->count() < _spriteAtlas.size()) {
        ui->atlasPreviewTabWidget->addTab(new SpriteAtlasPreview(this), "tab");
    }

    for (auto i=0; i<_spriteAtlas.size(); ++i) {
        const SpriteAtlas& atlas = _spriteAtlas[i];
        SpriteAtlasPreview* spriteAtlasPreview = qobject_cast<SpriteAtlasPreview*>(ui->atlasPreviewTabWidget->widget(i));
        if (spriteAtlasPreview) {
            QString title = QString().number(atlas.scale()*100) + "%";

            ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
            if (scalingVariantWidget && !scalingVariantWidget->name().isEmpty()) {
                title += QString(", %1").arg(scalingVariantWidget->name());
            }

            ui->atlasPreviewTabWidget->setTabText(i, title);
            spriteAtlasPreview->setAtlas(atlas,
                                         pixelFormatFromString(ui->pixelFormatComboBox->currentText()),
                                         ui->premultipliedCheckBox->isChecked());
        }
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

    _blockUISignals = true;
    ui->algorithmComboBox->setCurrentText(projectFile->algorithm());
    ui->trimModeComboBox->setCurrentText(projectFile->trimMode());
    ui->trimSpinBox->setValue(projectFile->trimThreshold());
    ui->epsilonHorizontalSlider->setValue(projectFile->epsilon() * 10);
    ui->textureBorderSpinBox->setValue(projectFile->textureBorder());
    ui->spriteBorderSpinBox->setValue(projectFile->spriteBorder());
    ui->dataFormatComboBox->setCurrentText(projectFile->dataFormat());
    ui->destPathLineEdit->setText(projectFile->destPath());
    ui->spriteSheetLineEdit->setText(projectFile->spriteSheetName());
    ui->imageFormatComboBox->setCurrentText(imageFormatToString(projectFile->imageFormat()));
    ui->pixelFormatComboBox->setCurrentText(pixelFormatToString(projectFile->pixelFormat()));
    ui->premultipliedCheckBox->setChecked(projectFile->premultiplied());
    ui->optModeComboBox->setCurrentText(projectFile->optMode());
    ui->optLevelSlider->setValue(projectFile->optLevel());

    while(ui->scalingVariantsGroupBox->layout()->count() > 0){
        QLayoutItem *item = ui->scalingVariantsGroupBox->layout()->takeAt(0);
        delete item->widget();
        delete item;
    }
    for (auto scalingVariant: projectFile->scalingVariants()) {
        ScalingVariantWidget* scalingVariantWidget = new ScalingVariantWidget(this,
                                                                              scalingVariant.name,
                                                                              scalingVariant.scale,
                                                                              scalingVariant.maxTextureSize,
                                                                              scalingVariant.pow2);
        connect(scalingVariantWidget, SIGNAL(remove()), this, SLOT(removeScalingVariant()));
        connect(scalingVariantWidget, SIGNAL(valueChanged(bool)), this, SLOT(scalingVariantWidgetValueChanged(bool)));
        ui->scalingVariantsGroupBox->layout()->addWidget(scalingVariantWidget);

        if (ui->scalingVariantsGroupBox->layout()->count() == 1) {
            scalingVariantWidget->setRemoveEnabled(false);
        }
    }

    _spritesTreeWidget->clear();
    _spritesTreeWidget->addContent(projectFile->srcList());

    _blockUISignals = false;
    refreshAtlas();

    // fit scene before open project
    SpriteAtlasPreview* spriteAtlasPreview = qobject_cast<SpriteAtlasPreview*>(ui->atlasPreviewTabWidget->currentWidget());
    if (spriteAtlasPreview) {
        spriteAtlasPreview->on_toolButtonZoomFit_clicked();
    }


    _currentProjectFileName = fileName;
    setWindowTitle(_currentProjectFileName + " - SpriteSheet Packer");

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
    if (!projectFile) {
        QMessageBox::critical(this, "ERROR", "Unknown project file format!");
        return;
    }

    projectFile->setAlgorithm(ui->algorithmComboBox->currentText());
    projectFile->setTrimMode(ui->trimModeComboBox->currentText());
    projectFile->setTrimThreshold(ui->trimSpinBox->value());
    projectFile->setEpsilon(ui->epsilonHorizontalSlider->value() / 10.f);
    projectFile->setTextureBorder(ui->textureBorderSpinBox->value());
    projectFile->setSpriteBorder(ui->spriteBorderSpinBox->value());
    projectFile->setDataFormat(ui->dataFormatComboBox->currentText());
    projectFile->setDestPath(ui->destPathLineEdit->text());
    projectFile->setSpriteSheetName(ui->spriteSheetLineEdit->text());
    projectFile->setImageFormat(imageFormatFromString(ui->imageFormatComboBox->currentText()));
    projectFile->setPixelFormat(pixelFormatFromString(ui->pixelFormatComboBox->currentText()));
    projectFile->setPremultiplied(ui->premultipliedCheckBox->isChecked());
    projectFile->setOptMode(ui->optModeComboBox->currentText());
    projectFile->setOptLevel(ui->optLevelSlider->value());

    QVector<ScalingVariant> scalingVariants;
    for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
        if (scalingVariantWidget) {
            ScalingVariant scalingVariant;
            scalingVariant.name = scalingVariantWidget->name();
            scalingVariant.scale = scalingVariantWidget->scale();
            scalingVariant.maxTextureSize = scalingVariantWidget->maxTextureSize();
            scalingVariant.pow2 = scalingVariantWidget->pow2();
            scalingVariants.push_back(scalingVariant);
        }
    }
    projectFile->setScalingVariants(scalingVariants);
    projectFile->setSrcList(_spritesTreeWidget->contentList());

    if (!projectFile->write(fileName)) {
        QMessageBox::critical(this, "ERROR", QString("Not support write to [%1] format.").arg(suffix.c_str()));
        return;
    }

    _currentProjectFileName = fileName;
    setWindowTitle(_currentProjectFileName + " - SpriteSheet Packer");

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

void MainWindow::setProjectDirty() {
    if (!_blockUISignals) {
        _projectDirty = true;
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()) {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) {
    QStringList filesList;
    for (auto url: event->mimeData()->urls()) {
        QFileInfo fi(url.toLocalFile());
        filesList.push_back(fi.canonicalFilePath());
    }
    if (filesList.size()) {
        _spritesTreeWidget->addContent(filesList);
        refreshAtlas();
    }
    event->accept();

    setProjectDirty();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    //alreadyClosed use only for fix bug on MacOS: https://bugreports.qt.io/browse/QTBUG-43344
    static bool alreadyClosed = false;
    if (_projectDirty && !alreadyClosed) {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Spritesheet Packer",
                                                                    tr("Your project file has been modified.\nDo you want to save your changes?"),
                                                                    QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                                    QMessageBox::Yes);
        if (resBtn == QMessageBox::Yes) {
            if (_currentProjectFileName.isEmpty()) {
                on_actionSaveAs_triggered();
            } else {
                saveSpritePackerProject(_currentProjectFileName);
            }
            event->accept();
            alreadyClosed = true;
        } else if (resBtn == QMessageBox::Cancel) {
            event->ignore();
        } else {
            event->accept();
            alreadyClosed = true;
        }
    }
}

void MainWindow::on_actionNew_triggered() {
    ui->spriteSheetLineEdit->setText("");
    _spritesTreeWidget->clear();

    _currentProjectFileName.clear();
    setWindowTitle("SpriteSheet Packer");

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

    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open project file."),
                                                    dir,
                                                    tr("Supported formats (*.json *.sp *.tps)"));
    qDebug() << selectedFilter;
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
    _projectDirty = false;
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
                                                    tr("Sprite sheet packer (*.json)"));
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
        settings.setValue("spritesPath", fileNames.back());

        _spritesTreeWidget->addContent(fileNames);
        refreshAtlas();
    }

    setProjectDirty();
}

void MainWindow::on_actionAddFolder_triggered() {
    QSettings settings;
    QString pathName = QFileDialog::getExistingDirectory(this,
                                                         tr("Open Folder"),
                                                         settings.value("spritesPath", QDir::currentPath()).toString(),
                                                         QFileDialog::DontResolveSymlinks);
    if (!pathName.isEmpty()) {
        settings.setValue("spritesPath", pathName);
        _spritesTreeWidget->addContent(QStringList() << pathName);
        refreshAtlas();
    }

    setProjectDirty();
}

void MainWindow::on_actionRemove_triggered() {
    foreach (QTreeWidgetItem* item, _spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            _spritesTreeWidget->removeItemWidget(item, 0);
            delete item;
        }
    }
    refreshAtlas();

    setProjectDirty();
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

    PublishSpriteSheet* publisher = new PublishSpriteSheet();
    publisher->setImageFormat(imageFormatFromString(ui->imageFormatComboBox->currentText()));
    publisher->setPixelFormat(pixelFormatFromString(ui->pixelFormatComboBox->currentText()));
    publisher->setPremultiplied(ui->premultipliedCheckBox->isChecked());

    PublishStatusDialog publishStatusDialog(this);
    publishStatusDialog.open();

    publishStatusDialog.log(QString("Publish to: " + dir.canonicalPath()), Qt::blue);

    if (_atlasDirty) {
        _spriteAtlas.clear();
    }
    for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
        if (scalingVariantWidget) {
            QString variantName = scalingVariantWidget->name();
            float scale = scalingVariantWidget->scale();
            int maxTextureSize = scalingVariantWidget->maxTextureSize();
            bool pow2 = scalingVariantWidget->pow2();

            QString spriteSheetName = ui->spriteSheetLineEdit->text();
            if (spriteSheetName.contains("{v}")) {
                spriteSheetName.replace("{v}", variantName);
            } else {
                spriteSheetName = variantName + spriteSheetName;
            }
            while (spriteSheetName.at(0) == '/') {
                spriteSheetName.remove(0,1);
            }

            QFileInfo destFileInfo;
            destFileInfo.setFile(dir, spriteSheetName);
            if (dir.absolutePath() != destFileInfo.dir().absolutePath()) {
                if (!dir.mkpath(destFileInfo.dir().absolutePath())) {
                    publishStatusDialog.log("Imposible create path:" + destFileInfo.dir().absolutePath(), Qt::red);
                    continue;
                }
            }

            if (!_atlasDirty) {
                publisher->addSpriteSheet(_spriteAtlas[i], destFileInfo.filePath());
                publishStatusDialog.log(QString("Get from cache scale variant (%1) scale: %2.").arg(spriteSheetName).arg(scale));
            } else {
                publishStatusDialog.log(QString("Generating scale variant (%1) scale: %2.").arg(spriteSheetName).arg(scale));

                SpriteAtlas atlas(_spritesTreeWidget->contentList(),
                                  ui->textureBorderSpinBox->value(),
                                  ui->spriteBorderSpinBox->value(),
                                  ui->trimSpinBox->value(),
                                  pow2,
                                  maxTextureSize,
                                  scale);

                atlas.setAlgorithm(ui->algorithmComboBox->currentText());

                if (ui->trimModeComboBox->currentText() == "Polygon") {
                    atlas.enablePolygonMode(true, ui->epsilonHorizontalSlider->value() / 10.f);
                }

                if (!atlas.generate()) {
                    QMessageBox::critical(this, "Generate error", "Max texture size limit is small!");
                    publishStatusDialog.log("Generate error: Max texture size limit is small!", Qt::red);
                    continue;
                } else {
                    _spriteAtlas.push_back(atlas);
                }

                publisher->addSpriteSheet(atlas, destFileInfo.filePath());
            }
        }
        refreshAtlas(false);
    }

    publisher->publish(ui->dataFormatComboBox->currentText(), ui->optModeComboBox->currentText(), ui->optLevelSlider->value());

    if (ui->optModeComboBox->currentText() == "None") {
        delete publisher;
    } else {
        // TODO: you need to wait previous image optimization if they are the same
        QObject::connect(publisher, &PublishSpriteSheet::onCompleted, [this, publisher] () {
            // TODO: it would be good to show here for information about the optimization
            QMessageBox::information(this, "PNG Optimization", "PNG Optimization: complete.");
            delete publisher;
        });
    }

    publishStatusDialog.log(QString("Publishing is finished."), Qt::blue);
    if (!publishStatusDialog.complete()) {
        // This loop will wait for the window is finished
        QEventLoop loop;
        connect(&publishStatusDialog, SIGNAL(finished(int)), &loop, SLOT(quit()));
        loop.exec();
        qDebug() << "Finish publish";
    }
}

void MainWindow::on_actionAbout_triggered() {
    AboutDialog aboutDialog(this);
    aboutDialog.exec();
}

void MainWindow::on_actionPreferences_triggered() {
    PreferencesDialog preferencesDialog(this);
    if (preferencesDialog.exec()) {
        refreshFormats();
    }
}

void MainWindow::spritesTreeWidgetItemSelectionChanged() {
    ui->actionRemove->setEnabled(false);
    foreach (QTreeWidgetItem* item, _spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            ui->actionRemove->setEnabled(true);
            return;
        }
    }
}

void MainWindow::on_destFolderToolButton_clicked() {
    QString destPath = ui->destPathLineEdit->text();
    destPath = QFileDialog::getExistingDirectory(this,
                                                 tr("Destination folder"),
                                                 destPath,
                                                 QFileDialog::DontResolveSymlinks);
    if (!destPath.isEmpty()) {
        ui->destPathLineEdit->setText(QDir(destPath).canonicalPath());
    }
}

void MainWindow::on_dataFormatSetupToolButton_clicked() {
    on_actionPreferences_triggered();
}

void MainWindow::on_addScalingVariantPushButton_clicked() {
    std::vector<std::pair<std::string, float>> defaultScaling = {
        std::make_pair("hdr/", 1),
        std::make_pair("hd/", 0.5f),
        std::make_pair("sd/", 0.25f),
    };

    unsigned int index = ui->scalingVariantsGroupBox->layout()->count();
    if (index >= defaultScaling.size()) {
        QMessageBox::warning(this, "Scaling variants", "Scaling variants is limited by 3.");
        return;
    }

    ScalingVariantWidget* scalingVariantWidget = new ScalingVariantWidget(this, defaultScaling[index].first.c_str(), defaultScaling[index].second);
    connect(scalingVariantWidget, SIGNAL(remove()), this, SLOT(removeScalingVariant()));
    connect(scalingVariantWidget, SIGNAL(valueChanged(bool)), this, SLOT(scalingVariantWidgetValueChanged(bool)));
    ui->scalingVariantsGroupBox->layout()->addWidget(scalingVariantWidget);

    if (ui->scalingVariantsGroupBox->layout()->count() == 1) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(0)->widget());
        if (scalingVariantWidget) {
           scalingVariantWidget->setRemoveEnabled(false);
        }
    } else{
        propertiesValueChanged();
        setProjectDirty();
    }
}

void MainWindow::removeScalingVariant() {
    qDebug() << "remove" << sender();
    ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(sender());
    if (scalingVariantWidget) {
        disconnect(scalingVariantWidget, SIGNAL(valueChanged(bool)), this, SLOT(scalingVariantWidgetValueChanged(bool)));
        ui->scalingVariantsGroupBox->layout()->removeWidget(scalingVariantWidget);
        delete scalingVariantWidget;
    }

    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::propertiesValueChanged() {
    if (_blockUISignals) return;
    QSettings settings;

    if (settings.value("MainWindow/automaticPreview", true).toBool()) {
        refreshAtlas();
    } else {
        _atlasDirty = true;
    }
}

void MainWindow::on_optModeComboBox_currentTextChanged(const QString &text) {
    bool visible = text == "Lossless";

    ui->optLevelSlider->setVisible(visible);
    ui->optLevelText->setVisible(visible);
    ui->optLevelLabel->setVisible(visible);

    setProjectDirty();
}

void MainWindow::on_optLevelSlider_valueChanged(int value) {
    ui->optLevelText->setText(QString::number(value));

    setProjectDirty();
}

void MainWindow::on_trimSpinBox_valueChanged(int) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_textureBorderSpinBox_valueChanged(int) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_spriteBorderSpinBox_valueChanged(int) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_epsilonHorizontalSlider_valueChanged(int) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_algorithmComboBox_currentTextChanged(const QString& text) {
    if (text == "Polygon") {
        ui->trimModeComboBox->setCurrentText("Polygon");
    }

    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_trimModeComboBox_currentIndexChanged(int) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_imageFormatComboBox_currentIndexChanged(int index) {
    auto setEnabledComboBoxItem = [](QComboBox* comboBox, int row, bool enable) {
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(comboBox->model());
        if (model) {
            QModelIndex index = model->index(row, comboBox->modelColumn());//, comboBox->rootModelIndex());
            QStandardItem* item = model->itemFromIndex(index);
            if (item) {
                item->setEnabled(enable);
            }
        }
    };
    ImageFormat imageFormat = (ImageFormat)index;
    if (imageFormat == kPNG) {
        ui->imageFormatStackedWidget->setVisible(true);
        ui->imageFormatStackedWidget->setCurrentIndex(0);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8888, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8565, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB4444, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB565, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kALPHA, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, false);
        if (ui->pixelFormatComboBox->currentIndex() > kARGB8888) {
            ui->pixelFormatComboBox->setCurrentIndex(kARGB8888);
        }
    } else if (imageFormat == kPKM) {
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB4444, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kALPHA, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, false);
        ui->pixelFormatComboBox->setCurrentIndex(kETC1);
    } else if ((imageFormat == kPVR)||(imageFormat == kPVR_CCZ)) {
        ui->imageFormatStackedWidget->setVisible(false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB4444, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kALPHA, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, true);
        if (ui->pixelFormatComboBox->currentIndex() < kPVRTC2) {
            ui->pixelFormatComboBox->setCurrentIndex(kPVRTC4);
        }
    }

    setProjectDirty();
}

void MainWindow::on_pixelFormatComboBox_currentIndexChanged(int index) {
    PixelFormat pixelFormat = (PixelFormat)index;
    if ((pixelFormat == kPVRTC2) ||
            (pixelFormat == kPVRTC2A) ||
            (pixelFormat == kPVRTC4) ||
            (pixelFormat == kPVRTC4A))
    {
        for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
            ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
            if (scalingVariantWidget) {
                scalingVariantWidget->setPow2(true);
                scalingVariantWidget->setEnabledPow2(false);
            }
        }
    } else {
        for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
            ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
            if (scalingVariantWidget) {
                scalingVariantWidget->setEnabledPow2(true);
            }
        }
    }

    QVector<PixelFormat> formatsWithAlpha = {kARGB8888, kARGB8565, kARGB4444, kPVRTC2A, kPVRTC4A};
    if (formatsWithAlpha.indexOf(pixelFormat) != -1) {
        ui->premultipliedCheckBox->show();
    } else {
        ui->premultipliedCheckBox->hide();
    }

    if (pixelFormat == kARGB8888) {
        ui->premultipliedCheckBox->setEnabled(true);
    } else {
        ui->premultipliedCheckBox->setChecked(true);
        ui->premultipliedCheckBox->setEnabled(false);
    }

    setProjectDirty();
    refreshPreview();
}

void MainWindow::on_dataFormatComboBox_currentIndexChanged(int) {
    setProjectDirty();
}

void MainWindow::on_destPathLineEdit_textChanged(const QString&) {
    setProjectDirty();
}

void MainWindow::on_spriteSheetLineEdit_textChanged(const QString&) {
    setProjectDirty();
}

void MainWindow::on_premultipliedCheckBox_toggled(bool checked) {
    setProjectDirty();
}

void MainWindow::scalingVariantWidgetValueChanged(bool refresh) {
    if (refresh) {
        propertiesValueChanged();
    }
    setProjectDirty();
}
