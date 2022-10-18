/*
	studio.js
	The main application script which contains most of the logic.
	Load only after loading all dependencies.
	
    TODO: Remember to use correct useragent in JQUERY!
*/

/*
* The Studio object which contains many global variables.
* Many of these variables only get initialized here and get
* filled AFTER loading the preferences.
*/

var Studio = new Object();

/*
* User session information
*/

Studio.Username = "";
Studio.Password = "";
Studio.LoggedIn = false;

/*
* Version information
*/

Studio.Version = "";
Studio.BuildDate = "";

/*
* Pane browser
*/

Studio.Panes = new Object();

/***************************************************************/


//App.EnableConsole();


//vex.dialog.buttons.YES.text = 'Yes';
//vex.dialog.buttons.NO.text = 'No';

function setTitle(txt)
{
	/*
	if(typeof txt=="undefined" || txt=="")
		document.title = "Studio";
	
	document.title = txt + " - Studio";*/
}


function initializeStudio(callback) {
    callback();
}


var openTab = "";

function deselectAllTabs() {
    document.getElementById("tbApps").className = '';
    document.getElementById("tbProjects").className = '';
	document.getElementById("tbMyJobs").className = '';
    document.getElementById("tbAnalytics").className = '';
    //document.getElementById("tbSettings").className = '';
	
	$('#wndBtnSettings').removeClass( "activated" );

    document.getElementById("tbApps_pane").style.display = 'none';
    document.getElementById("tbProjects_pane").style.display = 'none';
	document.getElementById("tbMyJobs_pane").style.display = 'none';
    document.getElementById("tbAnalytics_pane").style.display = 'none';
    document.getElementById("tbSettings_pane").style.display = 'none';
}

function selectTab(tabName) {
    deselectAllTabs();
	var tab = document.getElementById(tabName);
  
	if (tab !== null)
		tab.className = 'tbSelected';
	
    document.getElementById(tabName + "_pane").style.display = 'block';
	openTab = tabName;
	
	updatePaneNavBtns();
	localStorage.setItem("LastTab", tabName);
}

function setupTabs() {
    var tbApps = document.getElementById("tbApps");
    var tbMyJobs = document.getElementById("tbMyJobs");
	var tbProjects = document.getElementById("tbProjects");
    var tbAnalytics = document.getElementById("tbAnalytics");
    var tbSettings = document.getElementById("tbSettings");

    tbApps.addEventListener("click", function () {
        selectTab(this.id);
		setTitle("Apps");
    });

    tbProjects.addEventListener("click", function () {
        selectTab(this.id);
		setTitle("Projects");	
    });

	tbMyJobs.addEventListener("click", function () {
		selectTab(this.id);
		setTitle("My Jobs");	
    });
	
    tbAnalytics.addEventListener("click", function () {
        selectTab(this.id);
		setTitle("Analytics");
        //document.getElementById("chatTopPane").style["-webkit-user-select"] = "all";
        //document.getElementById("chatTopPane").style["color"] = "red";
    });

    /*tbSettings.addEventListener("click", function () {
		setTitle("Settings");
        selectTab(this.id);
    });*/
}

function checkForUpdates()
{
	document.getElementById("settingsUpdateBtn").disabled = true;
	document.getElementById("updateSpanStatus").innerHTML = "Searching for updates...";
	
	setTimeout(function(){
		App.CheckForUpdates();
	}, 1000);
}

function onFinishUpdateCheck()
{
	document.getElementById("settingsUpdateBtn").disabled = false;
	document.getElementById("updateSpanStatus").innerHTML = "";
}

function showLoading() {
	$( document.getElementById("paneLoadingScreen") ).css("display", "block");
}

function hideLoading() {
	$( document.getElementById("paneLoadingScreen") ).css("display", "none");
}

//
// Pane navigation
//

function getActivePane()
{
	return openTab;
}

function verifyPaneObjExists(pane)
{
	if(typeof Studio.Panes[ pane ]=="undefined") {
		Studio.Panes[ pane ] = new Object();
		Studio.Panes[ pane ].History = [];
		Studio.Panes[ pane ].ForwardHistory = [];
	}
}

function pushToPaneHistory(pane, url, bClearForward)
{
	verifyPaneObjExists(pane);
	
	if(bClearForward)
		Studio.Panes[ pane ].ForwardHistory = [];
	
	// Don't remember history older than 10 visits ago
	if( Studio.Panes[ pane ].History.length >10 )
		Studio.Panes[ pane ].History.shift();
	
	Studio.Panes[ pane ].History.push( url );
	
	if(url!='project_new_task.html') {
		// Also save the current url to our local storage
		localStorage.setItem(pane, url);
	}
	
	// Save the project session with every navigation in Projects
	if(pane=='tbProjects'){
		saveProjectSession();
	}	
}

function goBackInPane(pane)
{	
	var length = Studio.Panes[ pane ].History.length;
	
	if(length > 1) {		
		// Move the last element from History to ForwardHistory
		Studio.Panes[ pane ].ForwardHistory.push(
			Studio.Panes[ pane ].History.pop()
		);
		
		var url = Studio.Panes[ pane ].History[ length-2 ];
		
		openInPane(url, pane, false)
		
		Studio.Panes[ pane ].History.pop();
	}
}

function goForwardInPane(pane)
{
	if(Studio.Panes[ pane ].ForwardHistory.length > 0) {
		// Remove and get the last element from ForwardHistory
		var url = Studio.Panes[ pane ].ForwardHistory.pop();
			
		// Now go to the last in the history array
		openInPane(url, pane, false);
	}
}

function updatePaneNavBtns()
{
	var activePane = getActivePane();
	verifyPaneObjExists(activePane);
	
	var btnBack = document.getElementById("btnBack");
	var btnForward = document.getElementById("btnForward");
	
	if(Studio.Panes[ activePane ].History.length > 1)
		btnBack.disabled = false;
	else
		btnBack.disabled = true;	
	
	if(Studio.Panes[ activePane ].ForwardHistory.length > 0)
		btnForward.disabled = false;
	else
		btnForward.disabled = true;
}

function refreshInPane(pane)
{
	// TODO: Refreshing adds a new element in the history? Fix.
	openInPane(Studio.Panes[ pane ].History[ Studio.Panes[ pane ].History.length-1 ], pane)
}

function openInPane(url, pane, bClearForward)
{
	if(typeof pane=="undefined")
		pane = openTab;
	
	if (typeof bClearForward=="undefined")
		bClearForward = true;
		
	$.ajax({
		type: "GET",
		url : url,
		dataType : 'html',
	}).done(function(data){	
	
		$(  document.getElementById(pane + "_pane") ).html( data );
		
		
		pushToPaneHistory( pane, url, bClearForward );
		updatePaneNavBtns();
		
	}).fail(function(data){	
	
		$(  document.getElementById(pane + "_pane") ).html( "<div style=\"padding:40px;\"><h1>Feature not available</h1> \
			<p>This feature has not been developed yet. If this is an error, maybe you could try updating your client.</p></div> \
		" );
		
		pushToPaneHistory( pane, url, bClearForward );
		updatePaneNavBtns();		
	});	
}

function autoLoadPanes()
{
	//
	// Load the pages into their panes.
	// Previously loaded pages will load in-place.
	//
	
	hideLoading();
	
	if(localStorage.getItem("tbApps")!=null)
		openInPane(localStorage.getItem("tbApps"), "tbApps")
	else
		openInPane("apps.html", "tbApps");
	
	/*if(localStorage.getItem("tbProjects")!=null)
		openInPane(localStorage.getItem("tbProjects"), "tbProjects")
	else
		openInPane("projects.html", "tbProjects");*/
	
	restoreProjectSession();
	
	if(localStorage.getItem("tbMyJobs")!=null)
		openInPane(localStorage.getItem("tbMyJobs"), "tbMyJobs")
	else
		openInPane("my_jobs.html", "tbMyJobs");	
	
	if(localStorage.getItem("tbAnalytics")!=null)
		openInPane(localStorage.getItem("tbAnalytics"), "tbAnalytics")
	else
		openInPane("analytics.html", "tbAnalytics");	

	if(localStorage.getItem("tbSettings")!=null)
		openInPane(localStorage.getItem("tbSettings"), "tbSettings")
	else
		openInPane("settings.html", "tbSettings");		
}

function setupWindow() {
	$('#titlebar').hover(function(e){
		if($(e.target).is('nav')){
			cursorLeaveTitleBar();
			e.stopPropagation();
		}else if($(e.target).is('#titlebar')){
			cursorEnterTitleBar();
		}
	}, function(){
		cursorLeaveTitleBar();
	});
	
	$('#windowBtns').hover(function(){
		cursorLeaveTitleBar();
	}, function(){
		cursorEnterTitleBar();
	});
	
	$('#windowBtns a').hover(function(){
		$( this ).addClass( "selected" );
	}, function(){
		$( this ).removeClass( "selected" );
	});
	
	$('#mainNavContainer').hover(function(e){
		if($(e.target).is('nav')){
			// your child span is being hovered over
			e.stopPropagation();
		}else if($(e.target).is('#mainNavContainer')){
			cursorEnterTitleBar();
		}
	}, function(){
		cursorLeaveTitleBar();
	});
	
	
	$("#wndBtnSettings").click(function(){
		selectTab('tbSettings');
		$(this).addClass("activated");
	});
	
	$("#wndBtnMin").click(function(){
		App.MinimizeWindow();
	});
	
	$("#wndBtnMaxRestore").click(function(){
		App.MaxRestoreWindow();
	});	
	
	$("#wndBtnClose").click(function(){
		App.Exit();
	});		
}

function cursorEnterTitleBar() {
	App.EnterTitleBar();
}

function cursorLeaveTitleBar() {
	App.LeaveTitleBar();
}

function onMouseLeave( bMouseInWindow ) {
	$('#windowBtns a').removeClass( "selected" );
}

function onMaxRestore( state, borderSize ) {
	var SIZE_RESTORED   =   0;
	var SIZE_MAXIMIZED  =   2;
	
	if(state==SIZE_MAXIMIZED) {
		document.getElementById("wndBtnMaxRestore").style.backgroundImage = "url(/media/img/winBtnRestore.png)";
		// Our window's edges are cropped when maximized so we must pad our document.body to compensate.
		document.body.style.borderWidth = (borderSize-1) + "px";
	}else if(state==SIZE_RESTORED) {
		document.getElementById("wndBtnMaxRestore").style.backgroundImage = "url(/media/img/winBtnMaximize.png)";
		document.body.style.borderWidth = "1px";
	}
}

function onActivateWindow() {
	document.body.style.borderColor = "#00ffff";
}

function onDeactivateWindow() {
	document.body.style.borderColor = "#008888";
}

/***************************************************************/
// Entry point

function main() {	
	//
	// Get information from StudioLauncher
	//
	
    var json = JSON.parse(App.GetCmdLine());
    
    Studio.Username = json.Studio.Username;
    Studio.Password = json.Studio.Password;
    Studio.Version = json.Studio.Version;
    Studio.BuildDate = json.Studio.BuildDate;
	
	//
	// Setup stuff
	//
	
    setupTabs();
	
	var lastTab = localStorage.getItem("LastTab");
	
	if(lastTab!=null)
		selectTab(lastTab);
	else
		selectTab("tbProjects");
	
	// Custom-drawn window frame.
	setupWindow();

	// Load the proper html docs into the panes.
	autoLoadPanes();
	
	/*openInPane("projects.html", "tbProjects");
	openInPane("my_jobs.html", "tbMyJobs");
	openInPane("analytics.html", "tbAnalytics");
	openInPane("settings.html", "tbSettings");*/
	
	//
	// Misc
	//
	
    // Free Memory every minute
    setInterval(function () {
        App.FreeMemory();
    }, 60000);	
	
	//
	// Done
	//
	
	// Show the main window and remove the splashscreen!
	setTimeout(function () { App.ShowWindow(); }, 0);
}
