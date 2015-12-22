

function exportSpriteSheet(destPath, spriteSheetName, scalingVariant, spriteFrames) {
    var destFileName = destPath + "/";
    if (scalingVariant.folderName) {
        destFileName += scalingVariant.folderName + "/";
    }
    destFileName += spriteSheetName;

    console.log("Publish: " + destFileName);

    var plist = {};
    plist["metadata"] = {
        "format": 2,
        "textureFileName": spriteSheetName + ".png"
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

    writer.writeData(destFileName + ".plist", plist, "PLIST");
    writer.writeImage(destFileName + ".png");
}
