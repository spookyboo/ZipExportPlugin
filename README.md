# Zip Export Plugin

The __Zip Export Plugin__ is a plugin for the HLMS Editor and exports all materials in the material browser (all materials per project) to two json files (one for pbs and one for unlit)
and multiple texture (image) files and adds them all to a zip file called [_projectname_].zip  
This __Zip Export Plugin__ Github repository contains Visual Studio ZipExport.sln / ZipExport.vcxproj files for convenience (do not forget to change the properties to the correct include files, 
because it makes use of both HLMS Editor and Ogre3d include files).
The __Zip Export Plugin__ makes use of the generic plugin mechanism of Ogre3D.

**Installation:**  
Just add the plugin to the plugins.cfg file (under HLMSEditor/bin) with the entry Plugin=ZipExport; the HLMS Editor recognizes it if you have followed the rules above.