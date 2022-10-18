// (c) Copyright 2015-2017 Wild Montage, Inc.

if (WScript.Arguments.Length > 0) {
	var service = new ActiveXObject( "Schedule.Service" );
	service.Connect();

	try{
		var studioFolder = service.GetFolder("\\Wild Montage\\Studio");
		studioFolder.DeleteTask("Check Studio", 0);
	}catch(err){
	}
}else{
	WScript.Echo("This script is intended to be opened by Studio only.");
}

// End of file