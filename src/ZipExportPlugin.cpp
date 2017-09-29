/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  -----------------------------------------------------------------------------
*/

#include "OgreRoot.h"
#include "ZipExportPlugin.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsJson.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"
#include "OgreItem.h"
#include "zip.h"
#include <iostream>
#include <fstream>

namespace Ogre
{
#ifdef __APPLE__
	// In darwin and perhaps other BSD variants off_t is a 64 bit value, hence no need for specific 64 bit functions
#define FOPEN_FUNC(filename, mode) fopen(filename, mode)
#define FTELLO_FUNC(stream) ftello(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko(stream, offset, origin)
#else
#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define FTELLO_FUNC(stream) ftello64(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)
#endif
//#define WRITEBUFFERSIZE (32768)
#define WRITEBUFFERSIZE (131072)

	static const String gImportMenuText = "";
	static const String gExportMenuText = "Export materialbrowser to Ogre3d zip";
	static String gTempString = "";
	//---------------------------------------------------------------------
	ZipExportPlugin::ZipExportPlugin()
    {
    }
    //---------------------------------------------------------------------
    const String& ZipExportPlugin::getName() const
    {
        return GENERAL_HLMS_PLUGIN_NAME;
    }
    //---------------------------------------------------------------------
    void ZipExportPlugin::install()
    {
    }
    //---------------------------------------------------------------------
    void ZipExportPlugin::initialise()
    {
        // nothing to do
	}
    //---------------------------------------------------------------------
    void ZipExportPlugin::shutdown()
    {
        // nothing to do
    }
    //---------------------------------------------------------------------
    void ZipExportPlugin::uninstall()
    {
    }
	//---------------------------------------------------------------------
	bool ZipExportPlugin::isImport (void) const
	{
		return false;
	}
	//---------------------------------------------------------------------
	bool ZipExportPlugin::isExport (void) const
	{
		return true;
	}
	//---------------------------------------------------------------------
	void ZipExportPlugin::performPreImportActions(void)
	{
		// Nothing to do
	}
	//---------------------------------------------------------------------
	void ZipExportPlugin::performPostImportActions(void)
	{
		// Nothing to do
	}
	//---------------------------------------------------------------------
	void ZipExportPlugin::performPreExportActions(void)
	{
		// Nothing to do
	}
	//---------------------------------------------------------------------
	void ZipExportPlugin::performPostExportActions(void)
	{
		// Delete the copied files
		// Note: Deleting the files as part of the export and just after creating a zip file
		// results in a corrupted zip file. Apparently, the files are still in use, even if the zip file
		// is already closed
		mySleep(1);
		std::vector<String>::iterator it = mFileNamesDestination.begin();
		std::vector<String>::iterator itEnd = mFileNamesDestination.end();
		String fileName;
		while (it != itEnd)
		{
			fileName = *it;
			std::remove(fileName.c_str());
			++it;
		}
	}
	//---------------------------------------------------------------------
	unsigned int ZipExportPlugin::getActionFlag(void)
	{
		// 1. Open a dialog to directory were the exported files are saved
		// 2. The HLMS Editor passes all texture filenames used by the datablocks in the material browser to the plugin
		// 3. Delete all datablocks before the exporting is performed
		return	PAF_PRE_EXPORT_OPEN_DIR_DIALOG |
			PAF_PRE_EXPORT_TEXTURES_USED_BY_DATABLOCK |
			PAF_PRE_EXPORT_DELETE_ALL_DATABLOCKS;
	}
	//---------------------------------------------------------------------
	const String& ZipExportPlugin::getImportMenuText (void) const
	{
		return gImportMenuText;
	}
	//---------------------------------------------------------------------
	const String& ZipExportPlugin::getExportMenuText (void) const
	{
		return gExportMenuText;
	}
	//---------------------------------------------------------------------
	bool ZipExportPlugin::executeImport (HlmsEditorPluginData* data)
	{
		// nothing to do
		return true;
	}
	//---------------------------------------------------------------------
	bool ZipExportPlugin::executeExport (HlmsEditorPluginData* data)
	{
		mFileNamesDestination.clear();

		// Error in case no materials available
		if (data->mInMaterialFileNameVector.size() == 0)
		{
			data->mOutSuccessText = "executeExport: No materials to export";
			return true;
		}

		// Iterate through the json files of the material browser and load them into Ogre
		std::vector<String> materials;
		materials = data->mInMaterialFileNameVector;
		std::vector<String>::iterator it;
		std::vector<String>::iterator itStart = materials.begin();
		std::vector<String>::iterator itEnd = materials.end();
		String fileName;
		for (it = itStart; it != itEnd; ++it)
		{
			// Load the materials
			fileName = *it;
			if (fileName.empty())
			{
				data->mOutErrorText = "Trying to process a non-existing material filename";
				return false;
			}

			// If an Exception is thrown, it may be because the loaded material is already available; just ignore it
			if (!loadMaterial(fileName))
			{
				data->mOutErrorText = "Error while processing the materials";
				return false;
			}
		}

		// Retrieve all te texturenames from the loaded datablocks
		std::vector<String> v = data->mInTexturesUsedByDatablocks;
		if (data->mInTexturesUsedByDatablocks.size() == 0)
		{
			data->mOutErrorText = "No textures to export";
			return false;
		}

		// vector v only contains basenames; Get the full qualified name instead
		std::vector<String>::iterator itBaseNames;
		std::vector<String>::iterator itBaseNamesStart = v.begin();
		std::vector<String>::iterator itBaseNamesEnd = v.end();
		String baseName;
		std::vector<String> fileNamesSource;
		for (itBaseNames = itBaseNamesStart; itBaseNames != itBaseNamesEnd; ++itBaseNames)
		{
			baseName = *itBaseNames;

			// Search in the Ogre resources
			fileName = getFullFileNameFromResources(baseName, data);
			if (fileName.empty())
			{
				// It cannot be found in the resources, try it in the texture list from the project
				fileName = getFullFileNameFromTextureList(baseName, data);
			}

			if (!fileName.empty())
			{
				fileNamesSource.push_back(fileName);
				//LogManager::getSingleton().logMessage("Texture to export: " + fileName);
			}
		}

		// Copy all textures to the dir of the project file
		std::vector<String>::iterator itFileNamesSource;
		std::vector<String>::iterator itFileNamesSourceStart = fileNamesSource.begin();
		std::vector<String>::iterator itFileNamesSourceEnd = fileNamesSource.end();
		String fileNameSource;
		String fileNameDestination;
		for (itFileNamesSource = itFileNamesSourceStart; itFileNamesSource != itFileNamesSourceEnd; ++itFileNamesSource)
		{
			fileNameSource = *itFileNamesSource;
			std::ifstream src(fileNameSource, std::ios::binary);
			baseName = fileNameSource.substr(fileNameSource.find_last_of("/\\") + 1);
			fileNameDestination = data->mInExportPath + baseName;
			if (!isDestinationFileAvailableInVector(fileNameDestination))
				mFileNamesDestination.push_back(fileNameDestination);
			std::ofstream dst(fileNameDestination, std::ios::binary);
			dst << src.rdbuf();
			dst.close();
			src.close();
		}

		// Combine all currently created materials into one Json file
		Root* root = Root::getSingletonPtr();
		HlmsManager* hlmsManager = root->getHlmsManager();
		String exportPbsFileName = data->mInProjectName + ".pbs.material.json";
		String exportUnlitFileName = data->mInProjectName + ".unlit.material.json";
		hlmsManager->saveMaterials(HLMS_PBS, data->mInExportPath + exportPbsFileName);
		hlmsManager->saveMaterials(HLMS_UNLIT, data->mInExportPath + exportUnlitFileName);

		// Zip all files
		zipFile zf;
		int err;
		int errclose;
		const char* password = NULL;
		void* buf = NULL;
		int size_buf = 0;
		size_buf = WRITEBUFFERSIZE;
		int opt_compress_level = Z_DEFAULT_COMPRESSION;
		buf = (void*)malloc(WRITEBUFFERSIZE);
		if (buf == NULL)
		{
			LogManager::getSingleton().logMessage("ZipExportPlugin: Error allocating memory");
			return false;
		}
		String zipName = data->mInExportPath + data->mInProjectName + ".zip";
		char zipFile[1024];
		char filenameInZip[1024];
		strcpy(zipFile, zipName.c_str());

#ifdef USEWIN32IOAPI
		zlib_filefunc64_def ffunc;
		fill_win32_filefunc64A(&ffunc);
		zf = zipOpen2_64(zipFile, 0, NULL, &ffunc);
#else
		zf = zipOpen64(zipFile, 0);
#endif

		if (zf == NULL)
		{
			LogManager::getSingleton().logMessage("ZipExportPlugin: Error opening " + String(zipFile));
		}
		else
		{
			LogManager::getSingleton().logMessage("ZipExportPlugin: Creating  " + String(zipFile));

			// Add the copied textrue files to the zipfile and also add the json files to the mFileNamesDestination
			mFileNamesDestination.push_back(data->mInExportPath + exportPbsFileName);
			mFileNamesDestination.push_back(data->mInExportPath + exportUnlitFileName);
			std::vector<String>::iterator itDest = mFileNamesDestination.begin();
			std::vector<String>::iterator itDestEnd = mFileNamesDestination.end();
			while (itDest != itDestEnd)
			{
				fileNameDestination = *itDest;
				strcpy(filenameInZip, fileNameDestination.c_str());

				FILE* fin;
				int size_read;
				zip_fileinfo zi;
				unsigned long crcFile = 0;
				int zip64 = isLargeFile(filenameInZip);
				const char *savefilenameInZip;
				savefilenameInZip = filenameInZip;

				// The path name saved, should not include a leading slash.
				// If it did, windows/xp and dynazip couldn't read the zip file.
				while (savefilenameInZip[0] == '\\' || savefilenameInZip[0] == '/')
				{
					savefilenameInZip++;
				}

				// Strip the path
				const char *tmpptr;
				const char *lastslash = 0;
				for (tmpptr = savefilenameInZip; *tmpptr; tmpptr++)
				{
					if (*tmpptr == '\\' || *tmpptr == '/')
					{
						lastslash = tmpptr;
					}
				}
				if (lastslash != NULL)
				{
					savefilenameInZip = lastslash + 1; // base filename follows last slash.
				}

				err = zipOpenNewFileInZip3_64(zf, savefilenameInZip, &zi,
					NULL, 0, NULL, 0, NULL /* comment*/,
					(opt_compress_level != 0) ? Z_DEFLATED : 0,
					opt_compress_level, 0,
					/* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
					-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
					password, crcFile, zip64);

				if (err != ZIP_OK)
				{
					LogManager::getSingleton().logMessage("ZipExportPlugin: Error adding " + String(filenameInZip) + " to zipfile");
					return false;
				}
				else
				{
					fin = FOPEN_FUNC(filenameInZip, "rb");
					if (fin == NULL)
					{
						LogManager::getSingleton().logMessage("ZipExportPlugin: Error opening " + String(filenameInZip));
						return false;
					}
				}

				if (err == ZIP_OK)
				{
					do
					{
						err = ZIP_OK;
						size_read = (int)fread(buf, 1, size_buf, fin);
						if (size_read < size_buf)
							if (feof(fin) == 0)
							{
								LogManager::getSingleton().logMessage("ZipExportPlugin: Error reading " + String(filenameInZip));
								return false;
							}

						if (size_read > 0)
						{
							err = zipWriteInFileInZip(zf, buf, size_read);
							if (err < 0)
							{
								LogManager::getSingleton().logMessage("ZipExportPlugin: Error in writing " + String(filenameInZip) + " in zipfile");
								return false;
							}

						}
					} while ((err == ZIP_OK) && (size_read>0));
				}

				// Close the file that is added to the zip
				if (fin)
					fclose(fin);

				if (err < 0)
					err = ZIP_ERRNO;
				else
				{
					err = zipCloseFileInZip(zf);
					if (err != ZIP_OK)
					{
						LogManager::getSingleton().logMessage("ZipExportPlugin: Error in closing " + String(filenameInZip) + " in zipfile");
						return false;
					}
				}

				// Delete the file from the filesystem
				//std::remove(filenameInZip);

				// Next file
				++itDest;
			}
		}
		
		// Close the zipfile
		errclose = zipClose(zf, NULL);
		if (errclose != ZIP_OK)
		{
			LogManager::getSingleton().logMessage("ZipExportPlugin: Error in closing " + String(zipFile));
			return false;
		}

		free(buf);
		data->mOutReference = exportPbsFileName + " + " + exportUnlitFileName;
		data->mOutSuccessText = "Exported materials and json files to " + zipName;

		// Deleting the copied files here, results in a corrupted zip file, so put that as a separate post-export action
		return true;

		return true;
	}

	//---------------------------------------------------------------------
	bool ZipExportPlugin::loadMaterial(const String& fileName)
	{
		// Read the json file as text file and feed it to the HlmsManager::loadMaterials() function
		// Note, that the resources (textures, etc.) must be present (in resource loacation)

		Root* root = Root::getSingletonPtr();
		HlmsManager* hlmsManager = root->getHlmsManager();
		HlmsJson hlmsJson(hlmsManager);

		// Read the content of the file into a string/char*
		std::ifstream inFile;
		inFile.open(fileName);

		std::stringstream strStream;
		strStream << inFile.rdbuf();
		String jsonAsString = strStream.str();

		std::cout << jsonAsString << std::endl;
		inFile.close();
		const char* jsonAsChar = jsonAsString.c_str();

		try
		{
			// Load the datablocks (which also creates them)
			hlmsJson.loadMaterials(fileName,
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				jsonAsChar); // The fileName is only used for logging and has no purpose
		}
		
		// If an Exception is thrown, it may be because the loaded material is already available; just ignore it
		catch (Exception e){}
		
		return true;
	}

	//---------------------------------------------------------------------
	const String& ZipExportPlugin::getFullFileNameFromTextureList(const String& baseName, HlmsEditorPluginData* data)
	{
		gTempString = "";
		std::vector<String>::iterator it;
		std::vector<String>::iterator itStart = data->mInTextureFileNameVector.begin();
		std::vector<String>::iterator itEnd = data->mInTextureFileNameVector.end();
		String compareBaseName;
		String fileName;
		for (it = itStart; it != itEnd; ++it)
		{
			fileName = *it;
			//LogManager::getSingleton().logMessage("getFullFileNameFromTextureList - Texture in vector: " + fileName);
			compareBaseName = fileName.substr(fileName.find_last_of("/\\") + 1);
			if (baseName == compareBaseName)
			{
				gTempString = fileName;
				return gTempString;
			}
		}

		return gTempString;
	}

	//---------------------------------------------------------------------
	const String& ZipExportPlugin::getFullFileNameFromResources(const String& baseName, HlmsEditorPluginData* data)
	{
		// Only search in the default resource group, because that is the only group the HLMS Editor uses
		gTempString = "";
		String filename;
		String path;
		FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		FileInfoList::iterator it;
		FileInfoList::iterator itStart = list->begin();
		FileInfoList::iterator itEnd = list->end();
		for (it = itStart; it != itEnd; ++it)
		{
			FileInfo& fileInfo = (*it);
			if (fileInfo.basename == baseName)
			{
				gTempString = fileInfo.archive->getName() + "/" + baseName;
				return gTempString;
			}
		}
		
		return gTempString;
	}

	//---------------------------------------------------------------------
	int ZipExportPlugin::isLargeFile (const char* filename)
	{
		int largeFile = 0;
		ZPOS64_T pos = 0;
		FILE* pFile = FOPEN_FUNC(filename, "rb");

		if (pFile != NULL)
		{
			int n = FSEEKO_FUNC(pFile, 0, SEEK_END);
			pos = FTELLO_FUNC(pFile);

			printf("File : %s is %lld bytes\n", filename, pos);

			if (pos >= 0xffffffff)
				largeFile = 1;

			fclose(pFile);
		}

		return largeFile;
	}

	//---------------------------------------------------------------------
	bool ZipExportPlugin::isDestinationFileAvailableInVector(const String& fileName)
	{
		std::vector<String>::iterator itDest = mFileNamesDestination.begin();
		std::vector<String>::iterator itDestEnd = mFileNamesDestination.end();
		String compareFileName = fileName;
		Ogre::StringUtil::toUpperCase(compareFileName);
		String destFileName;
		while (itDest != itDestEnd)
		{
			destFileName = *itDest;
			Ogre::StringUtil::toUpperCase(destFileName);
			if (Ogre::StringUtil::match(compareFileName, destFileName))
				return true;

			itDest++;
		}

		return false;
	}

	//---------------------------------------------------------------------
	void ZipExportPlugin::mySleep(clock_t sec)
	{
		clock_t start_time = clock();
		clock_t end_time = sec * 1000 + start_time;
		while (clock() != end_time);
	}

}
