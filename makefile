## BeOS Generic Makefile v2.2 ##

## Fill in this file to specify the project being created, and the referenced
## makefile-engine will do all of the hard work for you.  This handles both
## Intel and PowerPC builds of the BeOS.

## Application Specific Settings ---------------------------------------------

# specify the name of the binary
NAME=Tolmach

# specify the type of binary
#	APP:	Application
#	SHARED:	Shared library or add-on
#	STATIC:	Static library archive
#	DRIVER: Kernel Driver
TYPE=APP

APP_MIME_SIG=x-vnd.Zyozik-Tolmach

#	add support for new Pe and Eddie features
#	to fill in generic makefile

#%{
# @src->@ 

#	specify the source files to use
#	full paths or paths relative to the makefile can be included
# 	all files, regardless of directory, will have their object
#	files created in the common object directory.
#	Note that this means this makefile will not work correctly
#	if two source files with the same name (source.c or source.cpp)
#	are included from different directories.  Also note that spaces
#	in folder names do not work well with this makefile.
SRCS=TolmachApp.cpp\
     Preferences.cpp\
     TolmachWin.cpp\
     PGBHandler.cpp\
     PGBIndex.cpp 

#     TolmachView.cpp\

#	specify the resource files to use
#	full path or a relative path to the resource file can be used.
RDEFS= Tolmach.rdef


LOCALES=be ru en uk

# @<-src@ 
#%}

#	end support for Pe and Eddie

#	specify additional libraries to link against
#	there are two acceptable forms of library specifications
#	-	if your library follows the naming pattern of:
#		libXXX.so or libXXX.a you can simply specify XXX
#		library: libbe.so entry: be
#		
#	- 	if your library does not follow the standard library
#		naming scheme you need to specify the path to the library
#		and it's name
#		library: my_lib.a entry: my_lib.a or path/my_lib.a
LIBS= root be textencoding locale localestub stdc++

#	specify additional paths to directories following the standard
#	libXXX.so or libXXX.a naming scheme.  You can specify full paths
#	or paths relative to the makefile.  The paths included may not
#	be recursive, so include all of the paths where libraries can
#	be found.  Directories where source files are found are
#	automatically included.
LIBPATHS= 

#	additional paths to look for system headers
#	thes use the form: #include <header>
#	source file directories are NOT auto-included here
SYSTEM_INCLUDE_PATHS = 

#	additional paths to look for local headers
#	thes use the form: #include "header"
#	source file directories are automatically included
LOCAL_INCLUDE_PATHS = 

#	specify the level of optimization that you desire
#	NONE, SOME, FULL
OPTIMIZE= NONE

#	specify any preprocessor symbols to be defined.  The symbols will not
#	have their values set automatically; you must supply the value (if any)
#	to use.  For example, setting DEFINES to "DEBUG=1" will cause the
#	compiler option "-DDEBUG=1" to be used.  Setting DEFINES to "DEBUG"
#	would pass "-DDEBUG" on the compiler's command line.
DEFINES=

#	specify special warning levels
#	if unspecified default warnings will be used
#	NONE = supress all warnings
#	ALL = enable all warnings
WARNINGS = 

#	specify whether image symbols will be created
#	so that stack crawls in the debugger are meaningful
#	if TRUE symbols will be created
SYMBOLS =

#	specify debug settings
#	if TRUE will allow application to be run from a source-level
#	debugger.  Note that this will disable all optimzation.
DEBUGGER =

#	specify additional compiler flags for all files
COMPILER_FLAGS =

#	specify additional linker flags
LINKER_FLAGS =

INSTALL_DIR = /boot/apps

#	specify the version of this particular item
#	(for example, -app 3 4 0 d 0 -short 340 -long "340 "`echo -n -e '\302\251'`"1999 GNU GPL") 
#	This may also be specified in a resource.
APP_VERSION =-app 3 4 0 d 0 -short 340 -long "340 "`echo -n -e '\302\251'`"1999 GNU GPL" 

#	(for TYPE == DRIVER only) Specify desired location of driver in the /dev
#	hierarchy. Used by the driverinstall rule. E.g., DRIVER_PATH = video/usb will
#	instruct the driverinstall rule to place a symlink to your driver's binary in
#	~/add-ons/kernel/drivers/dev/video/usb, so that your driver will appear at
#	/dev/video/usb when loaded. Default is "misc".
DRIVER_PATH = 

## include the makefile-engine
include $(BUILDHOME)/etc/makefile-engine

DIST_DIR := dist
APP_DEST := $(DIST_DIR)/common/bin/ 
LINK_DEST := $(DIST_DIR)/home/config/be/Application/
DICT_DEST := $(DIST_DIR)/common/data/Tolmach/
CATALOGS_DEST := $(DIST_DIR)/common/data/locale/catalogs/
VERSION := 1.1.0
DATE := `date +%F`
PACKAGE_NAME := $(NAME)-$(VERSION)-x86-gcc$(CC_VER)-$(DATE)

$(APP_DEST):
	mkdir -p $(APP_DEST)

$(LINK_DEST):
	mkdir -p $(LINK_DEST)

$(LINK_DEST)/$(NAME): $(LINK_DEST)
	ln -s -f /boot/common/bin/Tolmach $(LINK_DEST)/$(NAME)

$(DICT_DEST):
	mkdir -p $(DICT_DEST)

$(CATALOGS_DEST): $(OBJ_DIR)/$(APP_MIME_SIG)
	mkdir -p $(CATALOGS_DEST)

package: $(TARGET) $(APP_DEST) $(LINK_DEST)/$(NAME) $(DICT_DEST) $(CATALOGS_DEST) catalogs
	-cp $(TARGET) $(APP_DEST)
	-cp ./Dictionaries/* $(DICT_DEST)
	-cp -r $(OBJ_DIR)/$(APP_MIME_SIG) $(CATALOGS_DEST)
	echo "Package: $(NAME)" > $(DIST_DIR)/.OptionalPackageDescription
	echo "Version: $(VERSION)-gcc$(CC_VER)" >> $(DIST_DIR)/.OptionalPackageDescription
	echo "Copyright: Zyozik" >> $(DIST_DIR)/.OptionalPackageDescription
	echo "Description: Working shell for PGB dictionaries." >> $(DIST_DIR)/.OptionalPackageDescription
	echo "License: GPL" >> $(DIST_DIR)/.OptionalPackageDescription
	cd $(DIST_DIR) && zip -9 -r -z -y $(PACKAGE_NAME).zip common .OptionalPackageDescription < .OptionalPackageDescription

