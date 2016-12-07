

function exportSpriteSheet(dataFilePath, imageFilePaths, spriteFrames) {
    var imageFilePath = imageFilePaths.rgb || imageFilePaths;
    var maskFilePath = imageFilePaths.mask;
    var jsonFrames = {};
    for (var key in spriteFrames) {
        var spriteFrame = spriteFrames[key];

        var cocosFrame = {};
        cocosFrame["frame"] = {
            x: spriteFrame.frame.x,
            y: spriteFrame.frame.y,
            w: spriteFrame.frame.width,
            h: spriteFrame.frame.height,
        };
        cocosFrame["rotated"] = spriteFrame.rotated;
        cocosFrame["spriteSourceSize"] = {
            x: spriteFrame.sourceColorRect.x,
            y: spriteFrame.sourceColorRect.y,
            w: spriteFrame.sourceColorRect.width,
            h: spriteFrame.sourceColorRect.height
        };
        cocosFrame["sourceSize"] = {
            w: spriteFrame.sourceSize.width,
            h: spriteFrame.sourceSize.height
        };

        if ((spriteFrame.sourceSize.width !== spriteFrame.sourceColorRect.width) || (spriteFrame.sourceSize.height !== spriteFrame.sourceColorRect.height)) {
            cocosFrame["trimmed"] = true;
        } else {
            cocosFrame["trimmed"] = false;
        }

        jsonFrames[key] = cocosFrame;
    }

    var meta = {
        image: imageFilePath.replace(/^.*[\\\/]/, '')
    }

    if (maskFilePath) {
        meta.mask = maskFilePath.replace(/^.*[\\\/]/, '');
    }

    return {
        data: JSON.stringify({
                                 frames: jsonFrames,
                                 meta: meta
                             }, null, "\t"),
        format: "json"
    };
}
