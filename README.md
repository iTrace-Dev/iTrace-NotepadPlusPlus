# iTrace-NotepadPlusPlus
iTrace-NotepadPlusPlus (or iTrace-Notepad++) is a plugin for the Notepad++ Text Editor. The plugin will establish a connection to the [iTrace-Core](https://github.com/iTrace-Dev/iTrace-Core) desktop application. Once connected to the Core, the plugin will accept eye-tracking information from the Core and translate it to editor-specific data and output said data to an XML file.

# Installation
1. Download the latest version of the plugin. It will be a .dll file.
2. Open Notepad++, and select Plugins->Open Plugins Folder...
3. Create a folder for the plugin. The folder must be named the same as the .dll file. By default, the folder should be named `iTrace-NPP`.
4. Drag and drop the .dll file in the folder.
5. Restart Notepad++. The plugin should now be installed.

# Usage
To use iTrace-Notepad++, make sure you have iTrace-Core installed.
1. Open any files you wish to view in Notepad++.
2. Run iTrace-Core and set up the parameters of your tracking session.
3. Select Plugins->iTrace-NPP->Connect to iTrace Core. Alternatively, press `Alt+I`.
4. iTrace-Notepad++ should not be connected to iTrace-Core. A notification should appear in the bottom left notifying you of any errors in connecting.
5. Once a tracking session is started, iTrace-Notepad++ will begin writing to a file in the location specified in iTrace-Core. When the tracking session is finished, two files will be present - one from iTrace-Notepad++ and the other from iTrace-Core.

# How to Build From Source
If you want to build the plugin from source, follow these steps:
1. Download or clone the source code.
2. Open `/vs.proj/iTrace-NPP.vcxproj` using Visual Studio 2022.
3. The code should now be open in Visual Studio.
4. To build the .dll file, select Build->Build Solution. The .dll will be created in `/vs.proj/x64/Debug/`.

# Further Steps
After gathering your data, you can use our other tools [iTrace-Toolkit](https://github.com/iTrace-Dev/iTrace-Toolkit) and [iTrace-Visualize](https://github.com/iTrace-Dev/iTrace-Visualize) to analyze and process the tracking sessions.
