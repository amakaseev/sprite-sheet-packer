

function exportSpriteSheet(dataFilePath, imageFilePath, spriteFrames) {
    var plist = {};
    plist["metadata"] = {
        "format": 2,
        "textureFileName": imageFilePath.replace(/^.*[\\\/]/, '')
    };

    console.log("Collect spriteframes for cocos2d plist data");
    var cocosFrames = {};
    for (var key in spriteFrames) {
        var spriteFrame = spriteFrames[key];

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

    return {
        data: plist,
        format: "plist"
    };
}
