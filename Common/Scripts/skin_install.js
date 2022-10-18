var print = function(str){WScript.Echo(str);}
var fso = new ActiveXObject("Scripting.FileSystemObject");
var wshell = new ActiveXObject("WScript.Shell");

if (WScript.Arguments.Length != 1) {
	WScript.Echo("Bad input. Cannot register ActiveX class.");
	WScript.Quit(1)
}	

var dir = WScript.Arguments(0);
var ocxPath = dir+"\\bin\\Codejock.SkinFramework.v15.2.1.ocx";
var installPath = wshell.ExpandEnvironmentStrings( "%WINDIR%" ) + "\\System32\\";

// Move to system32
fso.CopyFile( ocxPath, installPath + "Codejock.SkinFramework.v15.2.1.ocx" );

// Register
var dir = WScript.Arguments(0);
var cmd = "regsvr32 /s Codejock.SkinFramework.v15.2.1.ocx";
wshell.Run(cmd, 0, true);  // 0 = hide window