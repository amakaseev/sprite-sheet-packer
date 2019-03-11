

function exportSpriteSheet(dataFilePath, imageFilePath, spriteFrames)
{
    var loopCount = 0;
    var contents = "";
    var imageList = "";
    var imageCount = Object.keys(spriteFrames).length;
    var spriteFrame;

    for (var key in spriteFrames)
    {
        spriteFrame = spriteFrames[key];
        imageList += "[sub_resource type=\"AtlasTexture\" id=" + (loopCount + 1) + "]\n";
        imageList += "atlas = ExtResource( 1 )\n";
        imageList += "region = Rect2( " + spriteFrame.frame.x + ", " +
                                          spriteFrame.frame.y + ", " +
                                          spriteFrame.frame.width + ", " +
                                          spriteFrame.frame.height + " )\n";
        imageList += "margin = Rect2( " + spriteFrame.sourceColorRect.x + ", " +
                                          spriteFrame.sourceColorRect.y + ", " +
                                          (spriteFrame.sourceSize.width - spriteFrame.frame.width) + ", " +
                                          (spriteFrame.sourceSize.height - spriteFrame.frame.height) + " )\n";
        imageList += "\n"
        loopCount++;
    }
    
    contents += "[gd_scene load_steps=" + (imageCount + 2) + " format=2]\n";
    contents += "\n";
    contents += "[ext_resource path=\"res://" + getFileName(imageFilePath) + "\" type=\"Texture\" id=1]\n";
    contents += "\n";
    contents += imageList;
    contents += "[node name=\"" + getFileNameWithoutExtension(imageFilePath) + "\" type=\"Sprite\"]\n\n";
    
    var partNumber = 1;
    
    for(var key in spriteFrames)
    {
        contents += "[node name=\"" + getFileNameWithoutExtension(key) + "\" type=\"Sprite\" parent=\".\"]\n";
        contents += "texture = SubResource(" + (partNumber) + ")\n\n";
        partNumber++;
    }
    contents += "\n";
    
    return {
        data: contents,
        format: "tscn"
    };
}


function getFileName(str)
{
    return str.split('\\').pop().split('/').pop();
}

function getFileNameWithoutExtension(str)
{
    return str.split('\\').pop().split('/').pop().split('.').shift();
}
