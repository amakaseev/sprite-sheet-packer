

function exportSpriteSheet(dataFilePath, imageFilePath, spriteFrames)
{
    var loopCount = 0;
    var contents = "";
    var imageList = "";
    var frameList = "";
    var imageCount = Object.keys(spriteFrames).length;
    var spriteFrame;
    var animationEntry = {};
    var previousAnimation = "";
  
    for (var key in spriteFrames)
    {
        currentAnimation = key.match(/^.+?(?=\/)/);
        
        if(currentAnimation == ".")
        {
            currentAnimation = "default";
        }
        
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
        loopCount++;
        frameList = "SubResource( " + loopCount + " ), ";
           
        if(previousAnimation.length == 0)                       // initial case
        {
            previousAnimation = currentAnimation;
        }
        
        if(animationEntry[currentAnimation] == undefined)       // if frameList is empty, copy
        {
            animationEntry[currentAnimation] = frameList;
        }
        else                                                    // if not, append
        {
            animationEntry[currentAnimation] += frameList;
        }
        
        if(strcmp(previousAnimation, currentAnimation) != 0)    // if it's different, a new animation
        {
            animationEntry[previousAnimation] = animationEntry[previousAnimation].slice(0, -2); // trim comma off
            frameList = "";                                     // wipe the frameList
        }
        
        if(loopCount == imageCount)                             // if it's the last
        {
            animationEntry[currentAnimation] = animationEntry[currentAnimation].slice(0, -2);   // trim comma off
        }
        
        previousAnimation = currentAnimation;
    }
    
    contents += "[gd_scene load_steps=" + (imageCount + 3) + " format=2]\n";
    contents += "\n";
    contents += "[ext_resource path=\"res://" + imageFilePath.replace(/^.*[\\\/]/, '') + "\" type=\"Texture\" id=1]\n";
    contents += "\n";
    contents += imageList;
    contents += "[sub_resource type=\"SpriteFrames\" id=" + (imageCount + 1) + "]\n";
    contents += "animations = [ ";
    
    loopCount = 0;
    var framecount = Object.keys(animationEntry).length
    for(var key in animationEntry)
    {
        contents += "{\n";
        contents += "\"frames\": [ " + animationEntry[key] + " ],\n";
        contents += "\"loop\": true,\n";
        contents += "\"name\": \"" + key + "\",\n";
        contents += "\"speed\": 5.0\n";
        contents += "}";
        
        loopCount++;
        if(loopCount < framecount)
        {
            contents += ", ";
        }
    }
    
    contents += " ]\n";
    contents += "\n";
    contents += "[node name=\"AnimatedSprite\" type=\"AnimatedSprite\"]\n";
    contents += "frames = SubResource( " + (imageCount + 1) + " )\n";
    contents += "frame = 0\n";
    contents += "\n";
    
    return {
        data: contents,
        format: "tscn"
    };
}

function strcmp(a, b)
{   
    return (a<b?-1:(a>b?1:0));  
}
