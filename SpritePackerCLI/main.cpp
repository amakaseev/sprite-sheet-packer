#include <QtCore>
//#include <QCoreApplication>
//#include <QCommandLineParser>

void printHelp() {
//    qInfo() <<  "Usage: ssp [source] [destination] [<*.png|gif|tif|jpg|swf|...>] [<imagefolder>] [<*.tps>*] ";
//    qInfo() << QDir::currentPath();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("SpriteSheetPacker");
    QCoreApplication::setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("source", "Source folder with sprites for packing.");
    parser.addPositionalArgument("destination", "Destination folder where saving the sprite sheet.");

    parser.addOptions({
        {"texture-border", "Border of the sprite sheet, value adds transparent pixels around the borders of the sprite sheet. Default value is 0.", "int", "0"},
        {"sprite-border", "Sprite border is the space between sprites. Value adds transparent pixels between sprites to avoid artifacts from neighbor sprites. The transparent pixels are not added to the sprites, default is 2.", "int", "2"},
        {"trim", "Allowed values: 1 to 255, default is 1. Pixels with an alpha value below this value will be considered transparent when trimming the sprite. Very useful for sprites with nearly invisible alpha pixels at the borders.", "int", "1"},
        {"powerOf2", "Forces the texture to have power of 2 size (32, 64, 128...). Default is disable."},
        {"max-size", "Sets the maximum size for the texture, default is 8192.", "size", "8192"},
        {"scale", "Scales all images before creating the sheet. E.g. use 0.5 for half size, default is 0.5", "float", "1"},
    });


    //parser.showHelp();
    parser.process(app);

    int textureBorder = 0;
    int spriteBorder = 2;
    int trim = 1;
    bool pot2 = false;
    int maxTextureSize = 8192;
    float scale = 1;
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
        pot2 = true;
    }
    if (parser.isSet("max-size")) {
        maxTextureSize = parser.value("max-size").toInt();
    }
    if (parser.isSet("scale")) {
        scale = parser.value("scale").toFloat();
    }

    const QStringList args = parser.positionalArguments();
    qDebug() << args;
    const QStringList options = parser.optionNames();
    qDebug() << options;

    qDebug() << "textureBorder:" << textureBorder;
    qDebug() << "spriteBorder:" << spriteBorder;
    qDebug() << "trim:" << trim;
    qDebug() << "pot2:" << pot2;
    qDebug() << "maxTextureSize:" << maxTextureSize;
    qDebug() << "scale:" << scale;

    return 1;
}

