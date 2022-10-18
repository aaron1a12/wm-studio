/*
=================================================================================================
version.js
Gets the version information specified in "Version.xml" and
generates a C++ header ("version.h").

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

var print = function(str){WScript.Echo(str);}

print("Generating version and build information...");

if (WScript.Arguments.Length != 2) {
	WScript.Echo("Bad input. Add a PRE-BUILD command like this: "+
	"cscript.exe C:\\WM_Studio\\VS2015\\Common\\Scripts\\version.js "+
	"\"$(SolutionDir)\" \"$(TargetFileName)\"");
	WScript.Quit(1)
}	

var solutionDir = WScript.Arguments(0);
var targetFileName = WScript.Arguments(1);

//
// Settings
//

var xmlPath = solutionDir + "Version.xml";
var headerTemplatePath = solutionDir + "Common\\Scripts\\version_h.template";
var resourceTemplatePath = solutionDir + "Common\\Scripts\\version_rc.template";
var headerPath = solutionDir + "Common\\version.h";
var resourcePath = solutionDir + "Common\\version.rc";

//
// Some important functions setups
//

var winWShell = new ActiveXObject( "WScript.Shell" );
var fso = new ActiveXObject("Scripting.FileSystemObject");
var xmlDoc = new ActiveXObject("Msxml2.DOMDocument.6.0");

var CurrentDirectory = fso.GetAbsolutePathName(".");
print(CurrentDirectory);

//
// Read the XML
//

xmlDoc.async = false;
xmlDoc.load( xmlPath );

if (xmlDoc.parseError.errorCode != 0) {
	var myErr = xmlDoc.parseError;
	WScript.Echo("XML parsing error: " + myErr.reason);
} else {
	NameTag = xmlDoc.getElementsByTagName("Name").item(0);
	ProductTag = xmlDoc.getElementsByTagName("Product").item(0);
	CompanyTag = xmlDoc.getElementsByTagName("Company").item(0);
	CopyrightTag = xmlDoc.getElementsByTagName("Copyright").item(0);
	WebsiteTag = xmlDoc.getElementsByTagName("Website").item(0);
	VersionTag = xmlDoc.getElementsByTagName("Version").item(0);
	BuildNumberTag = xmlDoc.getElementsByTagName("BuildNumber").item(0);
	BuildDateTag = xmlDoc.getElementsByTagName("BuildDate").item(0);
	
	// Increment build number
	BuildNumberTag.text++;
	
	// Set date
	var d = new Date();
	BuildDateTag.text = d.getTime();

	xmlDoc.save( xmlPath );
	
	//
	// Generate version.h
	//
	
	/*print(NameTag.text);
	print(CopyrightTag.text);
	print(VersionTag.text);
	print(BuildNumberTag.text);
	print(BuildDateTag.text);*/
	
	versionParts = VersionTag.text.split(".");
	
	
	var headerTemplate = fso.OpenTextFile( headerTemplatePath , 1);
	var headerTemplateStr = headerTemplate.ReadAll();
	headerTemplate.Close();
	
	headerTemplateStr = headerTemplateStr.replace("@MAJOR@", versionParts[0]);
	headerTemplateStr = headerTemplateStr.replace("@MINOR@", versionParts[1]);
	headerTemplateStr = headerTemplateStr.replace("@PATCH@", versionParts[2]);
	headerTemplateStr = headerTemplateStr.replace("@BUILDNUMBER@", BuildNumberTag.text);
	headerTemplateStr = headerTemplateStr.replace("@BUILDDATE@", BuildDateTag.text);
	headerTemplateStr = headerTemplateStr.replace("@FILENAME@", targetFileName);
	headerTemplateStr = headerTemplateStr.replace("@NAME@", NameTag.text);
	headerTemplateStr = headerTemplateStr.replace("@PRODUCT@", ProductTag.text);
	headerTemplateStr = headerTemplateStr.replace("@COMPANY@", CompanyTag.text);
	headerTemplateStr = headerTemplateStr.replace("@COPYRIGHT@", CopyrightTag.text);
	headerTemplateStr = headerTemplateStr.replace("@WEBSITE@", WebsiteTag.text);
	
	
	headerTemplateStr = "// Automatically-generated file. Edit the source in Common\\Scripts\\version_h.template\r\n\r\n" + headerTemplateStr
	
	versionHeaderFile = fso.OpenTextFile( headerPath, 2, true);
	versionHeaderFile.Write(headerTemplateStr);
	versionHeaderFile.Close();	
	
	//
	// Generate version.rc
	//
	
	/*
	var resourceTemplate = fso.OpenTextFile( resourceTemplatePath , 1);
	var resourceTemplateStr = resourceTemplate.ReadAll();
	resourceTemplate.Close();
	
	resourceTemplateStr = "// Automatically-generated file. Edit the source in Common\\Scripts\\version_rc.template\r\n\r\n" + resourceTemplateStr
	
	resourceFile = fso.OpenTextFile( resourcePath, 2, true);
	resourceFile.Write(resourceTemplateStr);
	resourceFile.Close();
	*/	
	
	print( "Done." );
	
}
 
//print( studioTag.baseName );
