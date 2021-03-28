#include <QtXml>
#include <QNetworkReply>

#include "MainWindow.h"
#include "SpriteAtlasPreview.h"
#include "PublishSpriteSheet.h"
#include "ScalingVariantWidget.h"
#include "AboutDialog.h"
#include "PreferencesDialog.h"
#include "PublishStatusDialog.h"
#include "AnimationDialog.h"
#include "ContentProtectionDialog.h"
#include "UpdaterDialog.h"
#include "ui_MainWindow.h"

#include "PListParser.h"
#include "PListSerializer.h"

#define MAX_RECENT 10

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

#if defined(Q_OS_WIN)
    setWindowIcon(QIcon("SpritePacker.ico"));
#elif defined(Q_OS_MAC)
    setWindowIcon(QIcon("SpritePacker.icns"));
#elif defined(Q_OS_LINUX)
    setWindowIcon(QIcon(":/res/icon.png"));
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
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kETC2));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kETC2A));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kPVRTC2));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kPVRTC2A));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kPVRTC4));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kPVRTC4A));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kDXT1));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kDXT3));
    ui->pixelFormatComboBox->addItem(pixelFormatToString(kDXT5));
    ui->pixelFormatComboBox->setCurrentIndex(0);
    ui->imageFormatComboBox->addItem(imageFormatToString(kPNG));
    ui->imageFormatComboBox->addItem(imageFormatToString(kWEBP));
    ui->imageFormatComboBox->addItem(imageFormatToString(kJPG));
    ui->imageFormatComboBox->addItem(imageFormatToString(kJPG_PNG));
    ui->imageFormatComboBox->addItem(imageFormatToString(kPKM));
    ui->imageFormatComboBox->addItem(imageFormatToString(kPVR));
    ui->imageFormatComboBox->addItem(imageFormatToString(kPVR_CCZ));
    ui->imageFormatComboBox->setCurrentIndex(0);

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

    ui->pngOptLevelSlider->setVisible(false);
    ui->pngOptLevelText->setVisible(false);
    ui->pngOptLevelLabel->setVisible(false);

    // layout preferences action on right side in toolbar
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->mainToolBar->insertWidget(ui->actionPreferences ,empty);

    _statusBarWidget = new StatusBarWidget(this);
    _statusBarWidget->showMessage("Ready.", QPixmap("://res/icon-ok.png"));
    ui->statusBar->addPermanentWidget(_statusBarWidget);

    createRefreshButton();
    setAcceptDrops(true);

    refreshFormats();
    refreshOpenRecentMenu();

    // reset dirty
    _projectDirty = false;
    _atlasDirty = false;

    QObject::connect(&_watcher, SIGNAL(started()), this, SLOT(onRefreshAtlasStarted()));
    QObject::connect(&_watcher, SIGNAL(finished()), this, SLOT(onRefreshAtlasCompleted()));

    QSettings settings;
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/state").toByteArray());

    //refreshAtlas();
    on_actionCheckForUpdates_triggered();
    
    // remove entries from sprite list with the DELETE key
    QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), _spritesTreeWidget);
    connect(shortcut, SIGNAL(activated()), this, SLOT(on_actionRemove_triggered()));
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
    QString fileName = senderAction->text();

    if (!_currentProjectFileName.isEmpty() && (fileName != _currentProjectFileName)) {
        MainWindow* wnd = new MainWindow();
        wnd->show();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        wnd->openSpritePackerProject(fileName);

    } else {
        openSpritePackerProject(fileName);
    }
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
    refreshButton->setIcon(QIcon(":/res/icon-refresh.png"));
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

        if (_future.isRunning()) {
            emit abortRefreshAtlas();
        }

        _future = QtConcurrent::run([this]() {
            _mutex.lock();
            _spriteAtlas.clear();
            for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
                ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
                if (scalingVariantWidget) {
                    float scale = scalingVariantWidget->scale();
                    int maxTextureSize = scalingVariantWidget->maxTextureSize();
                    bool pow2 = scalingVariantWidget->pow2();
                    bool forceSquared = scalingVariantWidget->forceSquared();

                    SpriteAtlas atlas = SpriteAtlas(_spritesTreeWidget->contentList(),
                                                    ui->textureBorderSpinBox->value(),
                                                    ui->spriteBorderSpinBox->value(),
                                                    ui->trimSpinBox->value(),
                                                    ui->heuristicMaskCheckBox->isChecked(),
                                                    pow2,
                                                    forceSquared,
                                                    maxTextureSize,
                                                    scale);

                    atlas.setRotateSprites(ui->rotateSpritesCheckBox->isChecked());
                    atlas.setAlgorithm(ui->algorithmComboBox->currentText());

                    if (ui->trimModeComboBox->currentText() == "Polygon") {
                        atlas.enablePolygonMode(true, ui->epsilonHorizontalSlider->value() / 10.f);
                    }

                    SpriteAtlasGenerateProgress* progress = new SpriteAtlasGenerateProgress();
                    connect(progress, SIGNAL(progressTextChanged(const QString&)), this, SLOT(onRefreshAtlasProgressTextChanged(const QString&)));

                    auto connection = connect(this, &MainWindow::abortRefreshAtlas, [&atlas]() {
                        atlas.abortGeneration();
                    });

                    if (!atlas.generate(progress)) {
                        disconnect(connection);
                        delete progress;
                        _mutex.unlock();
                        return false;
                    } else {
                        _spriteAtlas.push_back(atlas);
                    }
                    disconnect(connection);
                    delete progress;
                }
            }
            _atlasDirty = false;
            _mutex.unlock();
            return true;
        });
        _watcher.setFuture(_future);
    } else {
        onRefreshAtlasCompleted();
    }
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

    // show info if contents is empty
    if (!_spritesTreeWidget->contentList().size()) {
        ui->previewStackedWidget->setCurrentIndex(0);
    } else {
        ui->previewStackedWidget->setCurrentIndex(1);
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
    QSettings settings;
    settings.setValue("spritePackerFileName", fileName);

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
    ui->heuristicMaskCheckBox->setChecked(projectFile->heuristicMask());
    ui->rotateSpritesCheckBox->setChecked(projectFile->rotateSprites());
    ui->textureBorderSpinBox->setValue(projectFile->textureBorder());
    ui->spriteBorderSpinBox->setValue(projectFile->spriteBorder());
    ui->dataFormatComboBox->setCurrentText(projectFile->dataFormat());
    ui->destPathLineEdit->setText(projectFile->destPath());
    ui->spriteSheetLineEdit->setText(projectFile->spriteSheetName());
    ui->imageFormatComboBox->setCurrentText(imageFormatToString(projectFile->imageFormat()));
    ui->pixelFormatComboBox->setCurrentText(pixelFormatToString(projectFile->pixelFormat()));
    ui->premultipliedCheckBox->setChecked(projectFile->premultiplied());
    ui->pngOptModeComboBox->setCurrentText(projectFile->pngOptMode());
    ui->pngOptLevelSlider->setValue(projectFile->pngOptLevel());
    ui->webpQualitySlider->setValue(projectFile->webpQuality());
    ui->jpgQualitySlider->setValue(projectFile->jpgQuality());

    ui->trimSpriteNamesCheckBox->setChecked(projectFile->trimSpriteNames());
    ui->prependSmartFolderNameCheckBox->setChecked(projectFile->prependSmartFolderName());

    _encryptionKey = projectFile->encryptionKey();
    ui->contentProtectionToolButton->setChecked(!_encryptionKey.isEmpty());

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
                                                                              scalingVariant.pow2,
                                                                              scalingVariant.forceSquared);
        connect(scalingVariantWidget, SIGNAL(remove()), this, SLOT(onRemoveScalingVariant()));
        connect(scalingVariantWidget, SIGNAL(valueChanged(bool)), this, SLOT(onScalingVariantWidgetValueChanged(bool)));
        ui->scalingVariantsGroupBox->layout()->addWidget(scalingVariantWidget);

        if (ui->scalingVariantsGroupBox->layout()->count() == 1) {
            scalingVariantWidget->setRemoveEnabled(false);
        }
    }

    _spritesTreeWidget->clear();
    _spritesTreeWidget->addContent(projectFile->srcList());

    _blockUISignals = false;
    _needFitAfterRefresh = true;
    refreshAtlas();

    _currentProjectFileName = fileName;
    setWindowTitle(_currentProjectFileName + " - SpriteSheet Packer");

    // add to recent file
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
    projectFile->setHeuristicMask(ui->heuristicMaskCheckBox->isChecked());
    projectFile->setRotateSprites(ui->rotateSpritesCheckBox->isChecked());
    projectFile->setTextureBorder(ui->textureBorderSpinBox->value());
    projectFile->setSpriteBorder(ui->spriteBorderSpinBox->value());
    projectFile->setDataFormat(ui->dataFormatComboBox->currentText());
    projectFile->setDestPath(ui->destPathLineEdit->text());
    projectFile->setSpriteSheetName(ui->spriteSheetLineEdit->text());
    projectFile->setImageFormat(imageFormatFromString(ui->imageFormatComboBox->currentText()));
    projectFile->setPixelFormat(pixelFormatFromString(ui->pixelFormatComboBox->currentText()));
    projectFile->setPremultiplied(ui->premultipliedCheckBox->isChecked());
    projectFile->setPngOptMode(ui->pngOptModeComboBox->currentText());
    projectFile->setPngOptLevel(ui->pngOptLevelSlider->value());
    projectFile->setWebpQuality(ui->webpQualitySlider->value());
    projectFile->setJpgQuality(ui->jpgQualitySlider->value());
    projectFile->setTrimSpriteNames(ui->trimSpriteNamesCheckBox->isChecked());
    projectFile->setPrependSmartFolderName(ui->prependSmartFolderNameCheckBox->isChecked());
    projectFile->setEncryptionKey(_encryptionKey);

    QVector<ScalingVariant> scalingVariants;
    for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
        if (scalingVariantWidget) {
            ScalingVariant scalingVariant;
            scalingVariant.name = scalingVariantWidget->name();
            scalingVariant.scale = scalingVariantWidget->scale();
            scalingVariant.maxTextureSize = scalingVariantWidget->maxTextureSize();
            scalingVariant.pow2 = scalingVariantWidget->pow2();
            scalingVariant.forceSquared = scalingVariantWidget->forceSquared();
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
    MainWindow* wnd = new MainWindow();
    wnd->show();
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
                                                    tr("Supported formats (*.json *.ssp *.tps)"));

    if (!_currentProjectFileName.isEmpty() && !fileName.isEmpty() && (fileName != _currentProjectFileName)) {
        MainWindow* wnd = new MainWindow();
        wnd->show();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        wnd->openSpritePackerProject(fileName);
    } else if (!fileName.isEmpty()){
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
                                                    tr("Sprite sheet packer (*.ssp)"));
    QFileInfo info(fileName);
    if (!fileName.isEmpty()) {
        if (info.suffix() != "ssp") {
          fileName.append(".ssp");
        }
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
    publisher->setPngQuality(ui->pngOptModeComboBox->currentText(), ui->pngOptLevelSlider->value());
    publisher->setWebpQuality(ui->webpQualitySlider->value());
    publisher->setJpgQuality(ui->jpgQualitySlider->value());
    publisher->setTrimSpriteNames(ui->trimSpriteNamesCheckBox->isChecked());
    publisher->setPrependSmartFolderName(ui->prependSmartFolderNameCheckBox->isChecked());
    publisher->setEncryptionKey(_encryptionKey);

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
            bool forceSquared = scalingVariantWidget->forceSquared();

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
                                  forceSquared,
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

    publishStatusDialog.log("Publish data and images...", Qt::darkGreen);
    publisher->publish(ui->dataFormatComboBox->currentText());

    if (ui->pngOptModeComboBox->currentText() == "None") {
        delete publisher;
    } else {
        publishStatusDialog.log("PNG Optimization: optimize if needed.");
        // TODO: you need to wait previous image optimization if they are the same
        QObject::connect(publisher, &PublishSpriteSheet::onCompletedOptimizePNG, [this, publisher] () {
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

void MainWindow::on_actionPreferences_triggered() {
    PreferencesDialog preferencesDialog(this);
    if (preferencesDialog.exec()) {
        refreshFormats();
    }
}

void MainWindow::on_actionCheckForUpdates_triggered() {
    connect(&_networkManager, &QNetworkAccessManager::finished, [this](QNetworkReply* reply) {
        QUrl url = reply->url();
        if (reply->error()) {
            qDebug() << QString("Download of [%1] failed: %2").arg(url.toEncoded().constData()).arg(qPrintable(reply->errorString()));
        } else {
            qDebug() << QString("Download of [%1] succeeded").arg(url.toEncoded().constData());
            QString changelog = reply->readAll();
            QString version = changelog.split("\n", QString::SkipEmptyParts).at(0);
            int index = version.lastIndexOf('#');
            if (index != -1) {
                version = version.right(version.size() - index - 1);
            }
            qDebug() << "version:" << version;

            if (!version.isEmpty() && (version != QCoreApplication::applicationVersion())) {
                UpdaterDialog* dlg = new UpdaterDialog(version, changelog, this);
                dlg->exec();
            }
        }
        reply->deleteLater();
    });

    QUrl url = QUrl::fromEncoded("https://raw.githubusercontent.com/amakaseev/sprite-sheet-packer/master/CHANGELOG.md");
    _networkManager.get(QNetworkRequest(url));
}

void MainWindow::on_actionAbout_triggered() {
    AboutDialog aboutDialog(this);
    aboutDialog.exec();
}

void MainWindow::on_actionAnimation_triggered() {
    auto dlg = AnimationDialog::instance();
    if (!dlg) {
        dlg = new AnimationDialog(_spritesTreeWidget, this);
    }
    dlg->show();
    dlg->activateWindow();
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
    QSettings settings;
    QString destPath = ui->destPathLineEdit->text();
    destPath = QFileDialog::getExistingDirectory(this,
                                                 tr("Destination folder"),
                                                 settings.value("MainWindow/destinationFolder", QDir::currentPath()).toString(),
                                                 QFileDialog::DontResolveSymlinks);
    if (!destPath.isEmpty()) {
        ui->destPathLineEdit->setText(QDir(destPath).canonicalPath());
        settings.setValue("MainWindow/destinationFolder", destPath);
    }
}

void MainWindow::on_dataFormatSetupToolButton_clicked() {
    on_actionPreferences_triggered();
}

void MainWindow::on_contentProtectionToolButton_clicked() {
    ui->contentProtectionToolButton->setChecked(!_encryptionKey.isEmpty());

    ContentProtectionDialog* dlg = new ContentProtectionDialog(_encryptionKey, this);
    if (dlg->exec()) {
        _encryptionKey = dlg->encryptionKey();
        ui->contentProtectionToolButton->setChecked(!_encryptionKey.isEmpty());
    }
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
    connect(scalingVariantWidget, SIGNAL(remove()), this, SLOT(onRemoveScalingVariant()));
    connect(scalingVariantWidget, SIGNAL(valueChanged(bool)), this, SLOT(onScalingVariantWidgetValueChanged(bool)));
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

void MainWindow::on_pngOptModeComboBox_currentTextChanged(const QString &text) {
    bool visible = (text == "Lossless");

    ui->pngOptLevelSlider->setVisible(visible);
    ui->pngOptLevelText->setVisible(visible);
    ui->pngOptLevelLabel->setVisible(visible);

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

void MainWindow::on_epsilonHorizontalSlider_sliderMoved(int) {
    _epsilonValueChanged = true;
}

void MainWindow::on_epsilonHorizontalSlider_sliderReleased() {
    if (_epsilonValueChanged) {
        propertiesValueChanged();
        setProjectDirty();
        _epsilonValueChanged = false;
    }
}

void MainWindow::on_heuristicMaskCheckBox_toggled() {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_rotateSpritesCheckBox_toggled() {
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
    auto isEnabledComboBoxItem = [](QComboBox* comboBox, int row) {
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(comboBox->model());
        if (model) {
            QModelIndex index = model->index(row, comboBox->modelColumn());//, comboBox->rootModelIndex());
            QStandardItem* item = model->itemFromIndex(index);
            if (item) {
                return item->isEnabled();
            }
        }
        return false;
    };

    QTabBar* imageTabBar = ui->imageFormatSettingsTabWidget->tabBar();
    ImageFormat imageFormat = (ImageFormat)index;
    if ((imageFormat == kPNG) || (imageFormat == kWEBP)) {
        imageTabBar->setTabEnabled(0, imageFormat == kPNG);
        imageTabBar->setTabEnabled(1, imageFormat == kWEBP);
        imageTabBar->setTabEnabled(2, false);
        imageTabBar->setCurrentIndex((imageFormat == kPNG)? 0:1);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8888, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8565, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB4444, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB565, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kALPHA, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT3, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT5, false);
        if (!isEnabledComboBoxItem(ui->pixelFormatComboBox, ui->pixelFormatComboBox->currentIndex())) {
            ui->pixelFormatComboBox->setCurrentIndex(kARGB8888);
        }
    } else if (imageFormat == kJPG) {
        imageTabBar->setTabEnabled(0, false);
        imageTabBar->setTabEnabled(1, false);
        imageTabBar->setTabEnabled(2, true);
        imageTabBar->setCurrentIndex(2);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB4444, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB565, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kALPHA, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT3, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT5, false);
        if (!isEnabledComboBoxItem(ui->pixelFormatComboBox, ui->pixelFormatComboBox->currentIndex())) {
            ui->pixelFormatComboBox->setCurrentIndex(kRGB888);
        }
    } else if (imageFormat == kJPG_PNG) {
        imageTabBar->setTabEnabled(0, true);
        imageTabBar->setTabEnabled(1, false);
        imageTabBar->setTabEnabled(2, true);
        imageTabBar->setCurrentIndex(2);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB4444, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB565, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kALPHA, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT3, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT5, false);
        if (!isEnabledComboBoxItem(ui->pixelFormatComboBox, ui->pixelFormatComboBox->currentIndex())) {
            ui->pixelFormatComboBox->setCurrentIndex(kRGB888);
        }
    } else if (imageFormat == kPKM) {
        imageTabBar->setTabEnabled(0, false);
        imageTabBar->setTabEnabled(1, false);
        imageTabBar->setTabEnabled(2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB4444, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kALPHA, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2A, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT3, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT5, false);
        if (!isEnabledComboBoxItem(ui->pixelFormatComboBox, ui->pixelFormatComboBox->currentIndex())) {
            ui->pixelFormatComboBox->setCurrentIndex(kETC1);
        }
    } else if ((imageFormat == kPVR)||(imageFormat == kPVR_CCZ)) {
        imageTabBar->setTabEnabled(0, false);
        imageTabBar->setTabEnabled(1, false);
        imageTabBar->setTabEnabled(2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8888, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB8565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kARGB4444, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB565, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kALPHA, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC2A, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT1, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT3, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kDXT5, true);
        if (!isEnabledComboBoxItem(ui->pixelFormatComboBox, ui->pixelFormatComboBox->currentIndex())) {
            ui->pixelFormatComboBox->setCurrentIndex(kPVRTC4A);
        }
    }

    // enable/disable tabs content
    bool anyEnabled = false;
    for (int tab=0; tab<ui->imageFormatSettingsTabWidget->count(); ++tab) {
        ui->imageFormatSettingsTabWidget->widget(tab)->setEnabled(imageTabBar->isTabEnabled(tab));
        if (imageTabBar->isTabEnabled(tab)) anyEnabled = true;
    }
    ui->imageFormatSettingsTabWidget->setVisible(anyEnabled);

    setProjectDirty();
}

void MainWindow::on_pixelFormatComboBox_currentIndexChanged(int index) {
    bool needUpdatePreview = true;
    PixelFormat pixelFormat = (PixelFormat)index;
    if ((pixelFormat == kPVRTC2) ||
        (pixelFormat == kPVRTC2A) ||
        (pixelFormat == kPVRTC4) ||
        (pixelFormat == kPVRTC4A) ||
        (pixelFormat == kDXT1) ||
        (pixelFormat == kDXT3) ||
        (pixelFormat == kDXT5))
    {
        for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
            ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
            if (scalingVariantWidget) {
                scalingVariantWidget->setPow2(true);
                scalingVariantWidget->setEnabledPow2(false);
                needUpdatePreview = false;
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

    QVector<PixelFormat> formatsWithAlpha = {kARGB8888, kARGB8565, kARGB4444, kETC2A, kPVRTC2A, kPVRTC4A, kDXT1, kDXT3, kDXT5};
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
    if (needUpdatePreview) {
        refreshPreview();
    }
}

void MainWindow::on_dataFormatComboBox_currentIndexChanged(int) {
    setProjectDirty();
}

void MainWindow::on_destPathLineEdit_textChanged(const QString&) {
    setProjectDirty();
}

void MainWindow::on_spriteSheetLineEdit_textChanged(const QString&) {
    validatedSpriteSheetLineEdit();
    setProjectDirty();
}

void MainWindow::on_premultipliedCheckBox_toggled() {
    setProjectDirty();
}

void MainWindow::onScalingVariantWidgetValueChanged(bool refresh) {
    if (refresh) {
        propertiesValueChanged();
    }
    setProjectDirty();
}

void MainWindow::onRemoveScalingVariant() {
    qDebug() << "remove" << sender();
    ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(sender());
    if (scalingVariantWidget) {
        disconnect(scalingVariantWidget, SIGNAL(valueChanged(bool)), this, SLOT(onScalingVariantWidgetValueChanged(bool)));
        ui->scalingVariantsGroupBox->layout()->removeWidget(scalingVariantWidget);
        delete scalingVariantWidget;
    }

    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::onRefreshAtlasStarted() {
    qDebug() << "onRefreshAtlasStarted";
    _statusBarWidget->showSpinner("Generate atlas...");
}

void MainWindow::onRefreshAtlasCompleted() {
    qDebug() << "onRefreshAtlasCompleted";
    _statusBarWidget->hideSpinner();
    _statusBarWidget->showMessage("Finished.", QPixmap("://res/icon-ok.png"));

    refreshPreview();
    validatedSpriteSheetLineEdit();

    if (_needFitAfterRefresh) {
        // fit scene before open project
        for (auto i=0; i<ui->atlasPreviewTabWidget->count(); ++i) {
            SpriteAtlasPreview* spriteAtlasPreview = qobject_cast<SpriteAtlasPreview*>(ui->atlasPreviewTabWidget->widget(i));
            if (spriteAtlasPreview) {
                spriteAtlasPreview->on_toolButtonZoomFit_clicked();
            }
        }
        _needFitAfterRefresh = false;
    }
}

void MainWindow::onRefreshAtlasProgressTextChanged(const QString& message) {
    _statusBarWidget->showSpinner(message);
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

void MainWindow::validatedSpriteSheetLineEdit() {
    QPalette linePalette = ui->spriteSheetLineEdit->palette();

    bool isValide = true;

    if (!((ui->spriteSheetLineEdit->text().contains("{n}")) || (ui->spriteSheetLineEdit->text().contains("{n1}")))) {
        for (const auto& atlas: _spriteAtlas) {
            if (atlas.outputData().size() > 1) {
                isValide = false;
                break;
            }
        }
    }

    if ((_spriteAtlas.size() > 1) && (!ui->spriteSheetLineEdit->text().contains("{v}"))) {
        isValide = false;
    }

    if (!isValide) {
        linePalette.setColor(QPalette::Text, Qt::red);
    } else {
        linePalette.setColor(QPalette::Text, Qt::black);
    }

    ui->spriteSheetLineEdit->setPalette(linePalette);
}
