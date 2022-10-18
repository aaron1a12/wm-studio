// Automatically-generated file. Edit the source in Common\Scripts\version_h.template

/*
=================================================================================================
version.h or version_h.template
Version definitions. These can be modified in "Version.xml"

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#ifndef   STUDIO_VERSION_H
#define   STUDIO_VERSION_H


#ifdef _UNICODE
#define _T2(x)      L ## x
#else
#define _T2(x)      x
#endif

#define STRINGIZE2(s) _T2(#s)
#define STRINGIZE(s) STRINGIZE2(s)

#define VERSION_MAJOR               0
#define VERSION_MINOR               3
#define VERSION_REVISION            1
#define VERSION_BUILD               306

#if SVN_LOCAL_MODIFICATIONS
#define VERSION_MODIFIER _T2("M")
#else
#define VERSION_MODIFIER
#endif

#define VER_FILE_DESCRIPTION_STR    "Wild Montage Studio"
#define VER_FILE_VERSION            VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD
#define VER_FILE_VERSION_STR        STRINGIZE(VERSION_MAJOR)            \
                                    _T2(".") STRINGIZE(VERSION_MINOR)    \
                                    _T2(".") STRINGIZE(VERSION_REVISION) \
                                    _T2(".") STRINGIZE(VERSION_BUILD)    \
                                    VERSION_MODIFIER

#define VER_PRODUCTNAME_STR         "Wild Montage Studio"
#define VER_PRODUCT_VERSION         VER_FILE_VERSION
#define VER_PRODUCT_VERSION_STR     VER_FILE_VERSION_STR

#define VER_ORIGINAL_FILENAME_STR _T2("StudioLauncher.exe")
#define VER_INTERNAL_NAME_STR       VER_ORIGINAL_FILENAME_STR

#define VER_COPYRIGHT_STR           _T2("(c) Copyright 2015-2021 Wild Montage, LLC. All rights reserved.")
#define VER_COMPANY_STR             _T2("Wild Montage, Inc")

#ifdef _DEBUG
#define VER_VER_DEBUG             VS_FF_DEBUG
#else
#define VER_VER_DEBUG             0
#endif

#define VER_FILEOS                  VOS_NT_WINDOWS32
#define VER_FILEFLAGS               VER_VER_DEBUG

#if LIBRARY_EXPORTS
#define VER_FILETYPE              VFT_DLL
#else
#define VER_FILETYPE              VFT_APP
#endif

//
// Application strings
//

// Application version without build number
#define STUDIO_VERSION				STRINGIZE(VERSION_MAJOR)            \
                                    _T2(".") STRINGIZE(VERSION_MINOR)    \
                                    _T2(".") STRINGIZE(VERSION_REVISION) \
                                    VERSION_MODIFIER

// Application version with build number
#define STUDIO_VERSION_FULL			VER_FILE_VERSION_STR

#define STUDIO_BUILD_DATE			1626199665219

// Application name
#define STUDIO_NAME					VER_FILE_DESCRIPTION_STR

// Application copyright
#define STUDIO_COPYRIGHT			VER_COPYRIGHT_STR

// User agent for HTTP requests
#define STUDIO_USER_AGENT			_T2("Studio/") STRINGIZE(VERSION_MAJOR)            \
                                    _T2(".") STRINGIZE(VERSION_MINOR)    \
                                    _T2(".") STRINGIZE(VERSION_REVISION) \
									_T2(".") STRINGIZE(VERSION_BUILD)    \
                                    _T2(" (+https://www.wildmontage.com)")


#endif			
// End of File - Leave blank line
