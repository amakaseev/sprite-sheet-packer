

function exportSpriteSheet(destPath, spriteSheetName, scalingVariant, spriteFrames) {
    var destFileName = destPath + "/";
    if (scalingVariant.folderName) {
        destFileName += scalingVariant.folderName + "/";
    }
    destFileName += spriteSheetName;

    var jsonFrames = {};
    for (var key in spriteFrames) {
        var spriteFrame = spriteFrames[key];
        //console.log(" spriteFrame [" + key + "]" + JSON.stringify(spriteFrame));

        var cocosFrame = {};
        cocosFrame["frame"] = spriteFrame.frame;
        cocosFrame["sourceSize"] = spriteFrame.sourceSize;
        cocosFrame["rotated"] = spriteFrame.rotated;

        jsonFrames[key] = cocosFrame;
    }

    writer.writeData(destFileName + ".json", jsonFrames, "JSON");
    writer.writeImage(destFileName + ".png");
}
