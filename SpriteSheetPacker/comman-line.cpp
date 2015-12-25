#include <QtCore>
#include "SpriteAtlas.h"
#include "PublishSpriteSheet.h"

int commandLine(QCoreApplication& app) {
    QCommandLineParser parser;
    parser.setApplicationDescription("");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("source", "Source folder with sprites for packing.");
    parser.addPositionalArgument("destination", "Destination folder where saving the sprite sheet.");

    parser.addOptions({
        {{"f", "format"}, "Format for export sprite sheet data. Default is cocos2d.", "format"},
        {"texture-border", "Border of the sprite sheet, value adds transparent pixels around the borders of the sprite sheet. Default value is 0.", "int", "0"},
        {"sprite-border", "Sprite border is the space between sprites. Value adds transparent pixels between sprites to avoid artifacts from neighbor sprites. The transparent pixels are not added to the sprites, default is 2.", "int", "2"},
        {"trim", "Allowed values: 1 to 255, default is 1. Pixels with an alpha value below this value will be considered transparent when trimming the sprite. Very useful for sprites with nearly invisible alpha pixels at the borders.", "int", "1"},
        {"powerOf2", "Forces the texture to have power of 2 size (32, 64, 128...). Default is disable."},
        {"max-size", "Sets the maximum size for the texture, default is 8192.", "size", "8192"},
        {"scale", "Scales all images before creating the sheet. E.g. use 0.5 for half size, default is 0.5.", "float", "1"},
    });

    //--texture-border 10 /Users/alekseymakaseev/Documents/Work/run-and-jump/Assets/ART/Character /Users/alekseymakaseev/Documents/Work/run-and-jump/RunAndJump/testResources --trim 2
    parser.process(app);

    // check arguments
    if (parser.positionalArguments().size() != 2) {
        if (parser.positionalArguments().size() > 2) {
            qDebug() << "Too many arguments, see help for information.";
        } else {
            qDebug() << "Arguments must have source and destination, see help for information.";
        }
        parser.showHelp();
        return -1;
    }

    qDebug() << "arguments:" << parser.positionalArguments();
    qDebug() << "options:" << parser.optionNames();

    QFileInfo source(parser.positionalArguments().at(0));
    QFileInfo destination(parser.positionalArguments().at(1));
    if (!destination.isDir()) {
        qDebug() << "Incorrect destination folder";
        return -1;
    }

    // initialize [options]
    int textureBorder = 0;
    int spriteBorder = 2;
    int trim = 1;
    bool pow2 = false;
    int maxSize = 8192;
    float scale = 1;
    QString format = "cocos2d";
    if (parser.isSet("texture-border")) {
        textureBorder = parser.value("texture-border").toInt();
    }
    if (parser.isSet("sprite-border")) {
        spriteBorder = parser.value("sprite-border").toInt();
    }
    if (parser.isSet("trim")) {
        trim = parser.value("trim").toInt();
    }
    if (parser.isSet("powerOf2")) {
        pow2 = true;
    }
    if (parser.isSet("max-size")) {
        maxSize = parser.value("max-size").toInt();
    }
    if (parser.isSet("scale")) {
        scale = parser.value("scale").toFloat();
    }
    if (parser.isSet("format")) {
        format = parser.value("format");
    }

    qDebug() << "textureBorder:" << textureBorder;
    qDebug() << "spriteBorder:" << spriteBorder;
    qDebug() << "trim:" << trim;
    qDebug() << "pow2:" << pow2;
    qDebug() << "maxSize:" << maxSize;
    qDebug() << "scale:" << scale;

    // Generate sprite atlas
    SpriteAtlas atlas(QStringList() << source.filePath(), textureBorder, spriteBorder, trim, pow2, maxSize, scale);
    if (!atlas.generate()) {
        qCritical() << "ERROR: Generate atlass!";
        return -1;
    }

    // load formats
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
    qDebug() << "Support Formats:" << PublishSpriteSheet::formats().keys();


    // Publish data
    if (!PublishSpriteSheet::publish(destination.filePath() + "/" + source.fileName(), format, atlas, false)) {
        qCritical() << "ERROR: publish atlass!";
        return -1;
    }
    qDebug() << "Publishing is finished.";


//    qDebug() << source.fileName() << source.isDir();
//    qDebug() << destination.filePath() << destination.isDir();


    return 1;
}

