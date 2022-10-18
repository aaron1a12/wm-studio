/*
	analytics.js
	Scripts for the analytics tab
*/

function fetchScore()
{
	var score = App.GetProductivityScore();
	document.getElementById("spanScore").innerHTML = score;
}

var renderInterval = 0;
var renderTimeout = 10*60; // Render mode in seconds (10 mins.)
var renderCurTimeout;
var defaultRenderLabel = "Enable Render Mode for 10 mins."; 
function enableRenderMode( renderTimeLeft ) {
	var bResuming = false;
	
	if(typeof renderTimeLeft != 'undefined'){
		renderTimeout =	renderTimeLeft;
		bResuming = true;
	}
	
	renderCurTimeout = renderTimeout;
		
	// First disable the button
	var renderModeBtn = document.getElementById("renderModeBtn");
	renderModeBtn.disabled=true;	
	renderModeBtn.innerHTML = 'Enabling, please wait...';
	
	setTimeout(function(){
		var renderModeBtn = document.getElementById("renderModeBtn");
		
		if(bResuming) {
			var cpuAverage = 100;
		}else{
			var cpuUsage = Number(App.RunWait("wmi.bat"));
			var cpuUsage1 = Number(App.RunWait("wmi.bat"));
			var cpuUsage2 = Number(App.RunWait("wmi.bat"));
			
			var cpuAverage = (cpuUsage + cpuUsage1 + cpuUsage2) / 3;
		}
		
		if(cpuAverage < 10) {
			vex.dialog.alert('CPU usage is less than 10%. Please try again when rendering.');
			renderModeBtn.disabled=false;
			renderModeBtn.innerHTML = defaultRenderLabel;
		} else {
			//Studio.Analytics.RenderMode = true;
			if(!bResuming)
				App.EnableRenderMode();
			
			document.getElementById("scoreBg").style.backgroundColor = '#33221a';
			
			renderInterval = setInterval(function(){

				var renderModeBtn = document.getElementById("renderModeBtn");
				renderModeBtn.innerHTML = 'Render mode ends in ' + renderCurTimeout + ' seconds';
				
				if(renderCurTimeout==0){
					clearInterval(renderInterval);
					renderModeBtn.disabled = false;
					renderModeBtn.innerHTML = defaultRenderLabel;
					renderCurTimeout = renderTimeout;
					//Studio.Analytics.RenderMode = false;
					document.getElementById("scoreBg").style.backgroundColor = '#003333';
					
					// Round the final score
					//Studio.Analytics.Score = Math.round( Studio.Analytics.Score );
				}else{
					renderCurTimeout--;
				}
			}, 1000);
		}
	
	},200);	
}

function analyticsInit() {
	//
	// Analytics
	//
	
	// Fetch the amount of seconds left until render mode finishes	
	var renderTimeLeft = Number(App.GetRenderTime());
	
	// Resume render mode
	if(renderTimeLeft > 0)
		enableRenderMode( renderTimeLeft );
	
	// Fetch score
	fetchScore();
	setInterval(function () {
		fetchScore();
    }, 5000); // <-TODO: CHANGE TO 3000!
}