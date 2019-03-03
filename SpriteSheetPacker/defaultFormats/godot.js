

function exportSpriteSheet(dataFilePath, imageFilePath, spriteFrames)
{
    var loopCount = 0;
    var contents = "";
    var imageList = "";
    var frameList = "";
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
                                          spriteFrame.frame.height + ")\n";
        imageList += "margin = Rect2( " + spriteFrame.sourceColorRect.x + ", " +
                                          spriteFrame.sourceColorRect.y + ", " +
                                          (spriteFrame.sourceSize.width - spriteFrame.frame.width) + ", " +
                                          (spriteFrame.sourceSize.height - spriteFrame.frame.height) + " )\n";
        imageList += "\n"
        loopCount++
        frameList += "SubResource( " + loopCount + " )";
        
        if(loopCount < imageCount)
        {
            frameList += ", ";
        }
    }
    
    contents += "[gd_scene load_steps=" + (imageCount + 3) + " format=2]\n";
    contents += "\n";
    contents += "[ext_resource path=\"res://" + imageFilePath.replace(/^.*[\\\/]/, '') + "\" type=\"Texture\" id=1]\n";
    contents += "\n";
    contents += imageList;
    contents += "[sub_resource type=\"SpriteFrames\" id=" + (imageCount + 1) + "]\n";
    contents += "animations = [ {\n";
    contents += "\"frames\": [ " + frameList + " ],\n";
    contents += "\"loop\": false,\n";
    contents += "\"name\": \"default\",\n";
    contents += "\"speed\": 5.0\n";
    contents += "} ]\n";
    contents += "\n";
    contents += "[node name=\"AnimatedSprite\" type=\"AnimatedSprite\"]\n";
    contents += "frames = SubResource( " + (imageCount + 1) + " )\n";
    contents += "animation = \"default\"\n";
    contents += "frame = 0\n";
    contents += "\n";
    
    return {
        data: contents,
        format: "tscn"
    };
}
