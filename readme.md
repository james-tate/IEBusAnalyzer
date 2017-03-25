# IEBus analyzer using the Saleae Analyzer SDK Sample Analyzer

An IEBus analyzer for the digital side of an IEBus driver.
[The NEC PD72042B protocol documentation](http://elinux.org/images/e/e1/UPD72042.pdf) was used in the formatting of the analyzer.

To use: (building only tested on linux)
* cd IEBusAnalyzer
* git submodule init
* git submodule update
* python build_analyzer.py

For release
* copy /release/libIEBusAnalyzer.so to LogicX.X.X/Analyzers/
* open Saleae Logic x.x.x program

For debug
* open Saleae Logic x.x.x program
* choose Options->Preferences "upper right corner"
* under Developer tab set path to [yourpath]/IEBusAnalyzer/release
* save and restart Logic
* choose the IEBusAnalyzer under Analyzers->Show more analyzers

A prebuilt version of the analyzer is located in /prebuilt/

TODO:
	add example photos to readme before release
	make the print function work in the decoded protocols section of the GUI work
	handle parity and NAK errors (this will mainly be for GUI)

## Old Saleae Stuff

To build on Windows, open the visual studio project in the Visual Studio folder, and build. The Visual Studio solution has configurations for 32 bit and 64 bit builds. You will likely need to switch the configuration to 64 bit and build that in order to get the analyzer to load in the Windows software.

To build on Linux or OSX, run the build_analyzer.py script. The compiled libraries can be found in the newly created debug and release folders.

	python build_analyzer.py

To debug on Windows, please first review the article here:

[How do I develop custom analyzers for the Logic software on Windows?](http://support.saleae.com/hc/en-us/articles/208666946)

On Windows, it is not possible to attach a debugger to the latest software. In order to debug custom analyzers on Windows, you need to use an older version of the Windows software.
Specifically, you will need the Saleae Logic 1.1.18 Windows 32 bit standalone release. You can find it here:

[Older Saleae Logic Beta Downloads](http://support.saleae.com/hc/en-us/articles/210245603)

You will also need the 1.1.14 SDK, the last SDK release to support software versions 1.1.14-1.1.18. That is available on a separate branch of the Saleae AnalyzerSDK Github repository. Simply change the submodule configuration to point to that branch, and the existing visual studio project will just work. Be sure to only compile for 32 bit, since the 1.1.14 SDK was released before Saleae began targeting 64 bit Windows. This is also why the 32 bit standalone version of the 1.1.18 software is required.
It's also worth noting that you should use a #define and #ifdef to remove the contents of your Analyzer's GenerateFrameTabularText methods, because the methods ClearTabularText and AddTabularText were not yet present in the 1.1.14 SDK.

The Sample Analyzer Project also contains a branch, 1.1.14-for-win32-debug, which already contains the updated git submodule branch. Simply merge your changes into that branch, and then you will not need to switch submodule branches.

In the future, I would like to update this so that no branch changes are required, and the visual studio project just contains configurations for both the latest SDK and the 1.1.14 SDK.
