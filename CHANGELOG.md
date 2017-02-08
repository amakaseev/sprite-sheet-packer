##1.0.8
* Added [WebP](https://developers.google.com/speed/webp/) image format
* Added content protection in pvrtc
* Added support DXT1 DXT3 DXT5 pixel format and fixed ETC1(2)
* Supported multi window added by [TheCodez](https://github.com/TheCodez)

##1.0.7
* Update pngquant
* Removed webenginewidgets
* Added forceSquared option
* Added ETC2 and ETC2A pixel format
* Complete pvr.ccz compressed file format
* Created installer for Windows

##1.0.6
* Generate atlas in thread
* Added animation preview
* Added updater
* Created installer for MacOS

##1.0.5
* Added support multi-pack
	* {n}, {n1} - multipack index, starting with 0 or 1
* Added sprite atlas preview widget and show all scaling variants
* Added pixel formats: ARGB8565, ARGB4444, RGB565, ALPHA
* Added save to jpg file and refactor settings quality
* Added pixijs data format
* Added jpg(rgb)+png(alpha) format
* Added TrimSpriteNames and PrependSmartFolderName

##1.0.3
* Add save dialog on exit if project is dirty
* Implement polygon packing algorithm
* Implement generate polygon with sprites
* Add optipng and pngquant optimizer
* Add ToolTip for sprite frames
* Fix bug with copy image (break alpha chanel if image scale)
* Show identical list of sprite frame
* Win32: Polished publish dialog (Window title, position)
* Add display outlines on preview
* UI and command line implementation of image optimization
* Change zoom slider range (min: 10, max: 1000)
* Added automatically refreshed preview
* Generate spritesheet from commandline using existing project file

##1.0.2
* Implement drag and drop files and folders intro spritesheet from finder
* Open TexturePacker project format

##1.0.1
* Add command line

##1.0.0
* Add MIT License
* Add About dialog
* Create [site](http://amakaseev.github.io/sprite-sheet-packer)