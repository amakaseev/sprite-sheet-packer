

function exportSpriteSheet(dataFilePath, imageFilePath, spriteFrames, textureSize) {
    var plist = {};
    plist["metadata"] = {
        "format": 3,
        "textureFileName": imageFilePath.replace(/^.*[\\\/]/, '')
    };
    if (textureSize) {
        plist["metadata"]["size"] = "{" + textureSize.width + "," + textureSize.height + "}";
    }

    console.log("Collect spriteframes for cocos2d plist data");
    var cocosFrames = {};
    for (var key in spriteFrames) {
        var spriteFrame = spriteFrames[key];

        var cocosFrame = {};

        cocosFrame["aliases"] = [];

        cocosFrame["spriteSize"] = "{" +
                spriteFrame.frame.width + "," +
                spriteFrame.frame.height + "}";

        cocosFrame["spriteOffset"] = "{" +
                spriteFrame.offset.x + "," +
                spriteFrame.offset.y + "}";

        cocosFrame["spriteSourceSize"] = "{" +
                spriteFrame.sourceSize.width + "," +
                spriteFrame.sourceSize.height + "}";

        cocosFrame["textureRect"] = "{{" +
                spriteFrame.frame.x + "," +
                spriteFrame.frame.y + "},{" +
                spriteFrame.frame.width + "," +
                spriteFrame.frame.height + "}}";

        cocosFrame["textureRotated"] = spriteFrame.rotated;

        if (spriteFrame.triangles) {
            var triangles = "";
            var vertices = "";
            var verticesUV = "";
            for (var v in spriteFrame.triangles.verts) {
                var vtx = spriteFrame.triangles.verts[v];
                vertices += vtx.x + " " + vtx.y + " ";
                verticesUV += (spriteFrame.frame.x + vtx.x) + " " + (spriteFrame.frame.y + vtx.y) + " ";
            }
            for (var i in spriteFrame.triangles.indices) {
                triangles += spriteFrame.triangles.indices[i] + " ";
            }
            if (triangles.length) cocosFrame["triangles"] = triangles.slice(0, -1);
            if (vertices.length) cocosFrame["vertices"] = vertices.slice(0, -1);
            if (verticesUV.length) cocosFrame["verticesUV"] = verticesUV.slice(0, -1);
        }

        cocosFrames[key] = cocosFrame;
    }

    plist["frames"] = cocosFrames;

    return {
        data: plist,
        format: "plist"
    };
}
