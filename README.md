# Zip Export Plugin

The __Zip Export Plugin__ is a plugin for the HLMS Editor and exports all materials in the material browser, including all texture files to a zip file called [_projectname_].zip. 
This zipfile is written to the same directory of the _project.hlmp_ file  
This __Zip Export Plugin__ Github repository contains Visual Studio ZipExport.sln / ZipExport.vcxproj files for convenience (do not forget to change the properties to the correct include files, 
because it makes use of both HLMS Editor and Ogre3d include files).
The __Zip Export Plugin__ makes use of the generic plugin mechanism of Ogre3D.

**Installation:**  
Just add the plugin entry _Plugin=ZipExport_ to the plugins.cfg file (under HLMSEditor/bin); the HLMS Editor recognizes whether it is a valid plugin.