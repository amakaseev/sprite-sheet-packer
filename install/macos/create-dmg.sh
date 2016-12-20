#!/bin/sh
test -f SpriteSheetPacker-Installer.dmg && rm SpriteSheetPacker-Installer.dmg
appdmg appdmg.json SpriteSheetPacker-Installer.dmg
