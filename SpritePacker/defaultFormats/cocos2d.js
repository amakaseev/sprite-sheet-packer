

function exportSpriteSheet(filePath, spriteFrames) {

    var plist = {};
    plist["metadata"] = {
        "format": 2,
        "textureFileName": filePath.replace(/^.*[\\\/]/, '') + ".png"
    };

    var cocosFrames = {};
    for (var key in spriteFrames) {
        var spriteFrame = spriteFrames[key];
        //console.log(" spriteFrame [" + key + "]" + JSON.stringify(spriteFrame));

        var cocosFrame = {};
        cocosFrame["frame"] = "{{" +
                spriteFrame.frame.x + "," +
                spriteFrame.frame.y + "},{" +
                spriteFrame.frame.width + "," +
                spriteFrame.frame.height + "}}";

        cocosFrame["offset"] = "{" +
                spriteFrame.offset.x + "," +
                spriteFrame.offset.y + "}";

        cocosFrame["sourceSize"] = "{" +
                spriteFrame.sourceSize.width + "," +
                spriteFrame.sourceSize.height + "}";

        cocosFrame["rotated"] = spriteFrame.rotated;

        cocosFrames[key] = cocosFrame;
    }

    plist["frames"] = cocosFrames;

    writer.writeData(filePath + ".plist", plist, "PLIST");
    writer.writeImage(filePath + ".png");
}
