/*
	settings.js
	Scripts for the settings tab
*/

function settingsInit() {
	$.ajax({
		type: "GET",
		url : "http://studio.wildmontage.com/app/get-user-info.php",
		dataType : 'json',
		headers: {
        "Username":Studio.Username,
        "Password":Studio.Password
		}
	}).done(function(data){
		$( "[studio-fullname]" ).text( data.name + " " + data.lastname );
		
		document.getElementById("userPhoto").src = "http://media.esco.net/img/social/"+data.id+"/profile_small.jpg";
	});	
	
    var ver = Studio.Version;
    ver = ver.split(".");
    var build = ver.pop();
    ver = ver.join(".");


    var buildDate = new Date(Studio.BuildDate);

    document.getElementById("appVersion").innerHTML = ver + " (Build " + build + ")";

    document.getElementById("appBuildDate").innerHTML = buildDate;	
	
	refreshShutdownStatus();
}

function refreshShutdownStatus() {
	var span = document.getElementById("shtdwnStatus");
	
	if(App.GetShutDownStatus())
		span.innerHTML = "This workstation will shutdown tonight at 9:00 PM.";
	else
		span.innerHTML = "No shutdown will occur tonight.";
}

function toggleShutdown() {
	var span = document.getElementById("shtdwnStatus");

	if(App.GetShutDownStatus()) {
		App.SetShutDownStatus("1");
		span.innerHTML = "No shutdown will occur tonight.";
	} else {
		App.SetShutDownStatus("0");
		span.innerHTML = "This workstation will shutdown tonight at 9:00 PM.";
	}
}