/*
	projects.js
	Scripts for the projects tab
*/

//
// Project Session Global Variables
// Every single one gets saved into localStorage.
//

Studio.Project = new Object();
Studio.Project.LastUrl = "";
Studio.Project.Id = 0;
Studio.Project.Type = 0;
Studio.Project.Stage = 0;
Studio.Project.StageName = "";
Studio.Project.Task = 0;
Studio.Project.TaskName = "";
Studio.Project.Job = 0;
Studio.Project.JobName = ""; // Unused

//
// Other global variables. These are temporary per session.
//

var bEditingTask = false;
var bEditingJob = false;
var bUpdateJobThumb = false;
var jobScroll = 0;

//
// Methods for saving and restoring our session.
// Loading is done on start-up, while saving done with every pane navigation.
//

function saveProjectSession() {
	var count = Studio.Panes[ 'tbProjects' ].History.length;
	var lastUrl = Studio.Panes[ 'tbProjects' ].History[count-1];
	
	switch ( lastUrl ) {
		default:
			Studio.Project.LastUrl = lastUrl;

		// Don't save the following urls
		case "project_edit_task.html":
			break;
		case "project_edit_job.html":
			break; 
	}
	
	localStorage.setItem("ProjectSession", JSON.stringify(Studio.Project));
}

function restoreProjectSession() {
	var ProjectSession = localStorage.getItem("ProjectSession");

	if(ProjectSession!=null) {
		Studio.Project = JSON.parse( ProjectSession );
	}
	
	if(Studio.Project.LastUrl=="")
		Studio.Project.LastUrl = "projects.html";
	
	openInPane(Studio.Project.LastUrl, "tbProjects");
}


//
// Projects
//

function setActiveProject( projId, projType ) {
	if(typeof projId=="number") {
		Studio.Project.Id = projId;
	}else{
		Studio.Project.Id = 0;
	}

	localStorage.setItem("ActiveProject", Studio.Project.Id);
	
	if(typeof projType != "undefined") {
		Studio.Project.Type = projType;
		localStorage.setItem("ActiveProjectType", Studio.Project.Type);
	}else{
		Studio.Project.Type = 0;
	}
}

function projectsInit() {
	showLoading();
	$.ajax({
		type: "GET",
		url : "http://studio.wildmontage.com/app/project-get-list.php",
		dataType : 'json',
		headers: {
		"Username":Studio.Username,
		"Password":Studio.Password
		}
	}).done(function(data){
		hideLoading();
		var typeIcon;
		for (var key in data.projects) {
			switch(Number(data.projects[key][3])) {
				case 0:
					typeIcon = "local_movies";
					break;
				case 1:
					typeIcon = "work";
					break;
			}

			$( "#projectContainer" ).append("<div class='projectButton' style='margin-bottom:10px;' onclick=\"setActiveProject("+data.projects[ key ][0]+", "+data.projects[ key ][3]+"); openInPane('project_viewer.html', 'tbProjects');\"> \
				<img class=\"thumbnail\" src=\"http://media.studio.wildmontage.com/projects/"+data.projects[ key ][0]+"/project.jpg\"> \
				<div class=\"projectTitle\">"+data.projects[ key ][1]+"</div> \
				<div class=\"projectProgress\" style=\"width:"+data.projects[ key ][5]+"%;\"></div> \
				<i class=\"material-icons projectType\">"+typeIcon+"</i> \
				<div style=\"clear:both;\"></div> \
				</div>");
		}
		
		
		for (var key in data.completedProjects) {
			switch(Number(data.completedProjects[key][3])) {
				case 0:
					typeIcon = "local_movies";
					break;
				case 1:
					typeIcon = "work";
					break;
			}

			$( "#completedProjectContainer" ).append("<div class='projectButton' style='margin-bottom:10px;' onclick=\"setActiveProject("+data.completedProjects[ key ][0]+", "+data.completedProjects[ key ][3]+"); openInPane('project_viewer.html', 'tbProjects');\"> \
				<img class=\"thumbnail\" src=\"http://media.studio.wildmontage.com/projects/"+data.completedProjects[ key ][0]+"/project.jpg\"> \
				<div class=\"projectTitle\">"+data.completedProjects[ key ][1]+"</div> \
				<div class=\"projectProgress\" style=\"width:"+data.completedProjects[ key ][5]+"%;\"></div> \
				<i class=\"material-icons projectType\">"+typeIcon+"</i> \
				<div style=\"clear:both;\"></div> \
				</div>");
		}		
		
	}).fail(function(jqXHR, textStatus, errorThrown){
		hideLoading();
		alert('error: '+errorThrown);
	});
}

//
// Project viewer
//

function openProjectPage( url ) {	
	$.ajax({
		type: "GET",
		url : url,
		dataType : 'html',
	}).done(function(data){		
		$(  document.getElementById("projectViewerPage") ).html( data );
	}).fail(function(data){		
		$(  document.getElementById(pane + "_pane") ).html( "<div style=\"padding:40px;\"><h1>Feature not available</h1> \
			<p>This feature has not been developed yet. If this is an error, maybe you could try updating your client.</p></div> \
		" );
	});	
}

function projectViewerInit() {
	showLoading();

	/*// If the project stage was previously set, open the Tasks page
	if(Studio.Project.Stage!=0){
		openInPane('project_tasks.html', 'tbProjects');
	}
	else
	{*/
		var imgPoster = document.getElementById("projectPoster");
		
		$(imgPoster).load(function(){
			generateBgColor(this);
		});
		
		//imgPoster.src = "http://studio.wildmontage.com/app/project-get-thumbnail.php?id="+;
		imgPoster.src = "http://media.studio.wildmontage.com/projects/"+Studio.Project.Id+"/project.jpg";

		$.ajax({
			type: "POST",
			url : "http://studio.wildmontage.com/app/project-get-info.php",
			data: {
				"id":Studio.Project.Id
				},
			dataType : 'json',
			headers: {
			"Username":Studio.Username,
			"Password":Studio.Password
			}
		}).done(function(data){

			$("#projectTitle").text(data.project.name);
			$("#projectViewerDate").text(data.project.date);
			$("#projectViewerRunning").text(data.project.running);
			$("#projectViewerProg").text(data.project.progress+"%");
		
			$("#projectViewerProgBar").css("width", data.project.progress+"%");
			
			//openProjectPage("project_stages.html");
			//$("#foo").css("background-color", "red");
			//$("#projectViewerBgImage").css('background-image', 'url(http://studio.wildmontage.com/app/project-get-thumbnail.php?id='+data.project.id+')');
			hideLoading();
		}).fail(function(jqXHR, textStatus, errorThrown){
			hideLoading();
			alert('error: '+errorThrown);
		});		

		//
		// Get project stages
		//
		
		$.ajax({
			type: "POST",
			url : "http://studio.wildmontage.com/app/project-get-stages.php",
			data: {
				"project_id": Studio.Project.Id,
				"project_type": Studio.Project.Type
				},
			dataType : 'json',
			headers: {
			"Username":Studio.Username,
			"Password":Studio.Password
			}
		}).done(function(data){
			
			//alert(data);
			
			for (var stage_cat in data.stages) {
				var stageCatTitle = data.stages[stage_cat][0];
				
				
				$( "#projectStages" ).append( "<h3>"+stageCatTitle+"</h3>" );
				
				for (var stage in data.stages[stage_cat][1]) {

					var stageId = data.stages[stage_cat][1][stage][0];
					var stageTitle = data.stages[stage_cat][1][stage][1];
					var stageProgress = data.stages[stage_cat][1][stage][2];
					var bHasJobs = data.stages[stage_cat][1][stage][3];
					
					var divBtn = document.createElement("DIV");
					
					divBtn.className = "projectButton";
					divBtn.setAttribute("stage-id", stageId);
					divBtn.onclick = function(){
						setActiveProjectStage( Number(this.getAttribute('stage-id')) );
						openInPane('project_tasks.html', 'tbProjects');
					};
					var span = document.createElement("SPAN");
					
					var i = document.createElement("I");
					i.className = "material-icons";
					if(bHasJobs)
						i.appendChild( document.createTextNode( "folder" ) );
					else
						i.appendChild( document.createTextNode( "folder_open" ) );
					
					span.appendChild( i );					
					span.appendChild( document.createTextNode( " "+stageTitle ) );
					
					var progDiv = document.createElement("DIV");
					progDiv.className = "projectProgress";
					progDiv.style.width = stageProgress+"%";

					divBtn.appendChild( span );
					divBtn.appendChild( progDiv );
					
					
					$( "#projectStages" ).append( divBtn );
					
					/*<div class=''>
						<span><i class="material-icons">folder_open</i> Screenplay Writing</span>
						<div class="projectProgress" style="width:100%;"></div>
					</div>*/
					
					//alert( 'Stage ID:' + stageId + ', Title:'+ stageTitle)
				}
			}
			hideLoading();
			
		}).fail(function(jqXHR, textStatus, errorThrown){
			hideLoading();
			alert('error: '+errorThrown);
		});			
	

		
	//} // ENDOF else (Studio.Project.Stage==0)
}

function generateBgColor(imgEl) {
	var rgb = getAverageRGB(imgEl);

	rgb.r = rgb.r-10;
	rgb.g = rgb.g-10;
	rgb.b = rgb.b-10;
	
	$("#projectViewerContainer").css({
		//background: "rgba("+rgb.r+','+rgb.g+','+rgb.b+", 0.5)" 
		background: "-webkit-gradient(linear, left top, right bottom, from(#222), to(rgba("+rgb.r+','+rgb.g+','+rgb.b+", 1)))" 
	});
}

function setActiveProjectStage( stageId ) {
	if(typeof stageId=="number") {
		Studio.Project.Stage = stageId;
	}else{
		Studio.Project.Stage = 0;
	}

	localStorage.setItem("ActiveProjectStage", Studio.Project.Stage);
}

//
// Edit Task
//

function projectTasksCreateNew() {
	openInPane('project_edit_task.html', 'tbProjects');
}

function projectTasksEditActive() {
	bEditingTask = true;
	openInPane('project_edit_task.html', 'tbProjects');
}

function projectTaskLeaveEdit() {
	if(bEditingTask)
		bEditingTask = false;
	
	goBackInPane( getActivePane() );
}

function projectTaskSave() {
	var taskId = -1;
	var name = document.getElementById("inputNewTaskName").value;
	var desc = document.getElementById("inputNewTaskDesc").value;
	
	if(bEditingTask)
		taskId = Studio.Project.Task;
	
	if($("#inputNewTaskName").val()=="")
		return vex.dialog.alert("Please enter a name.");
	if($("#inputNewTaskDesc").val()=="")
		return vex.dialog.alert("Please enter a description for your task.");
	
	showLoading();
	$.ajax({
		type: "POST",
		url : "http://studio.wildmontage.com/app/project-edit-task.php",
		data: {
			"project_id": Studio.Project.Id,
			"project_stage": Studio.Project.Stage,
			"task_name": name,
			"task_desc": desc,
			"task_id": taskId
			},
		dataType : 'text',
		headers: {
		"Username":Studio.Username,
		"Password":Studio.Password
		}
	}).done(function(data){
		
		//alert(data);
		
		projectTaskLeaveEdit();
		hideLoading();
	}).fail(function(jqXHR, textStatus, errorThrown){
		hideLoading();
		alert('error: '+errorThrown);
	});		
}

function projectTaskDelete() {
	vex.dialog.confirm({
		message: 'Are you sure you wish to permanently delete the task?',
		callback: function (value) {
			if (value) {
				showLoading();
				$.ajax({
					type: "POST",
					url : "http://studio.wildmontage.com/app/project-edit-task.php",
					data: {
						"task_id": Studio.Project.Task,
						"project_id": Studio.Project.Id,
						"delete": 1
						},
					dataType : 'json',
					headers: {
					"Username":Studio.Username,
					"Password":Studio.Password
					}
				}).done(function(data){
					
					//alert(data);
					
					if(data.success==false) {
						vex.dialog.alert('Unable to delete task. Please verify there are no jobs in the task.');
					}else{
						bEditingTask = false;
						openInPane('project_tasks.html');
					}
					hideLoading();
				}).fail(function(jqXHR, textStatus, errorThrown){
					hideLoading();
					alert('error: '+errorThrown);
				});					
			}
		}
	});
}

function projectEditTaskInit() {
	$("#projectTaskFileUnder").text( Studio.Project.StageName );
	
	
	if(bEditingTask) {
		showLoading();
		$.ajax({
			type: "POST",
			url : "http://studio.wildmontage.com/app/project-get-task.php",
			data: {
				"task_id": Studio.Project.Task
				},
			dataType : 'json',
			headers: {
			"Username":Studio.Username,
			"Password":Studio.Password
			}
		}).done(function(data){

			$("#projectEditTaskH1").text( 'Editing task "'+data.task.name+'"' );
			$("#inputNewTaskName").val( data.task.name );
			$("#inputNewTaskDesc").val( data.task.desc );
			$("#projectTaskDeleteBtn").css("display", "inline-block");			
			hideLoading();
		}).fail(function(jqXHR, textStatus, errorThrown){
			hideLoading();
			alert('error: '+errorThrown);
		});
	}
}

//
// Tasks page
//

function projectTasksInit() {
	showLoading();
	// Studio.Project.Stage
	
	//
	// Get stage information
	//
	
	$.ajax({
		type: "POST",
		url : "http://studio.wildmontage.com/app/project-get-stage-and-tasks.php",
		data: {
			"project_id": Studio.Project.Id,
			"project_stage": Studio.Project.Stage
			},
		dataType : 'json',
		headers: {
		"Username":Studio.Username,
		"Password":Studio.Password
		}
	}).done(function(data){
		$("#projectStageName").text(data.stage.name);
		
		var totalJobsStr = data.stage.jobs + " job";
		if(data.stage.jobs==0 || data.stage.jobs > 1)
			totalJobsStr += "s";
		
		$("#projectStageProgress").text("("+data.stage.progress+"% done, "+totalJobsStr+")");
		
		Studio.Project.StageName = data.stage.name;
		
		for (var task in data.tasks) {
			var taskName = data.tasks[task][0];
			var taskDesc = data.tasks[task][1];
			var jobCount = data.tasks[task][2];
			
			var tRow = document.createElement("TR");
			
			var tNameCol = document.createElement("TD");
			
			var i = document.createElement("I");
			i.className = "material-icons";
			if(jobCount>0)
				i.appendChild( document.createTextNode( "folder" ) );
			else
				i.appendChild( document.createTextNode( "folder_open" ) );			
			tNameCol.appendChild( i );	
					
			tNameCol.appendChild( document.createTextNode( " "+taskName ) );
			var tDescCol = document.createElement("TD");
			tDescCol.appendChild( document.createTextNode( taskDesc ) );
			
			tRow.appendChild(tNameCol);
			tRow.appendChild(tDescCol);
			
			tRow.setAttribute("task-id", task);
			tRow.setAttribute("task-name", taskName);
			tRow.onclick = function() {
				Studio.Project.Task = Number( this.getAttribute("task-id") );
				Studio.Project.TaskName = this.getAttribute("task-name");
				openInPane('project_jobs.html', getActivePane());
			};
			
			$("#taskTable").append( tRow );
		}
		hideLoading();
		
	}).fail(function(jqXHR, textStatus, errorThrown){
		hideLoading();
		alert('error: '+errorThrown);
	});		
}

//
// Edit Job
//

function projectJobsCreateNew() {
	openInPane('project_edit_job.html', 'tbProjects');
}

function projectJobsEditActive() {
	bEditingJob = true;
	openInPane('project_edit_job.html', 'tbProjects');
}

function projectJobLeaveEdit() {
	Studio.Project.Job = 0;
	if(bEditingJob)
		bEditingJob = false;
	
	goBackInPane( getActivePane() );
}

function projectJobSave() {
	var jobId = -1;
	var name = document.getElementById("inputNewJobName").value;
	var desc = document.getElementById("inputNewJobDesc").value;
	var priority = document.getElementById("inputNewJobPriority").value;
	var updateThumb = -1;
	
	if(bEditingJob)
		jobId = Studio.Project.Job;
	
	if(bUpdateJobThumb)
		updateThumb = 1;
	
	if($("#inputNewJobName").val()=="")
		return vex.dialog.alert("Please enter a name.");
	if($("#inputNewJobDesc").val()=="")
		return vex.dialog.alert("Please enter a description for your job. If this is a VFX job, try including the filename of the shot.");	

	showLoading();
	$.ajax({
		type: "POST",
		url : "http://studio.wildmontage.com/app/project-edit-job.php",
		data: {
			"task_id": Studio.Project.Task,
			"job_id": jobId,
			"job_name": name,
			"job_desc": desc,
			"job_priority": priority,
			"job_update_thumb": updateThumb
			},
		dataType : 'text',
		headers: {
		"Username":Studio.Username,
		"Password":Studio.Password
		}
	}).done(function(data){
		
		//alert(data);
		
		projectJobLeaveEdit();
		hideLoading();
	}).fail(function(jqXHR, textStatus, errorThrown){
		hideLoading();
		alert('error: '+errorThrown);
	});
}

function projectJobDelete() {
	vex.dialog.confirm({
		message: 'Are you sure you wish to permanently delete the job?',
		callback: function (value) {
			if (value) {
				$.ajax({
					type: "POST",
					url : "http://studio.wildmontage.com/app/project-edit-job.php",
					data: {
						"job_id": Studio.Project.Job,
						"task_id": Studio.Project.Task,
						"delete": 1
						},
					dataType : 'text',
					headers: {
					"Username":Studio.Username,
					"Password":Studio.Password
					}
				}).done(function(data){
					
					//alert(data);
					
					if(data.success==false) {
						vex.dialog.alert('Unable to delete task. Please verify there are no jobs in the task.');
					}else{
						projectJobLeaveEdit();
					}
					
				}).fail(function(jqXHR, textStatus, errorThrown){
					alert('error: '+errorThrown);
				});					
			}
		}
	});
}

function projectEditJobUpload( input ) {
	if (window.FormData) {
		formdata = new FormData();
		
		var i = 0, len = input.files.length, img, reader, file;
		for ( ; i < len; i++ ) {
			file = input.files[i];

			if (!!file.type.match(/image.*/)) {
				formdata.append("images[]", file);
			} 
		}
	}
  
	if (formdata) {
		showLoading();
	
		$.ajax({
			url: "http://studio.wildmontage.com/app/project-upload-thumb.php",
			type: "POST",
			data: formdata,
			dataType : 'json',
			processData: false,
			contentType: false,
			headers: {
			"Username":Studio.Username,
			"Password":Studio.Password
			}			
		}).done(function(data){
			
			bUpdateJobThumb = true;
			
			App.ClearProjectCache();

			if(data.ok) {
				document.getElementById("jobThumbnail").src = data.thumbcache;
			}
	
			$("#fileBrowser").val("");
			hideLoading();			
			
		}).fail(function(jqXHR, textStatus, errorThrown){
			alert('error: '+errorThrown);
			hideLoading();
		});
	}
}

function projectEditJobInit() {
	bUpdateJobThumb = false;
	
	$("#jobThumbnail").attr("src","http://media.studio.wildmontage.com/projects/"+Studio.Project.Id+"/jobs/job_"+Studio.Project.Job+".jpg");
	
	$("#projectJobFileUnder").text( Studio.Project.TaskName );
	
	
	$("#projectEditJobThmbBtn").click(function() {
		$("#fileBrowser").click();
		//App.ProjectJobPickThumb('projectEditJob_pickThumbCallback');
    });
	
	$("#fileBrowser").change(function() {
		projectEditJobUpload( this );
    });
	
	
	if(bEditingJob) {	
		showLoading();
		$.ajax({
			type: "POST",
			url : "http://studio.wildmontage.com/app/project-get-job.php",
			data: {
				"job_id": Studio.Project.Job
				},
			dataType : 'json',
			headers: {
			"Username":Studio.Username,
			"Password":Studio.Password
			}
		}).done(function(data){

			$("#projectEditJobH1").text( 'Editing job "'+data.job.name+'"' );
			$("#inputNewJobName").val( data.job.name );
			$("#inputNewJobDesc").val( data.job.desc );
			$("#inputNewJobPriority").val( data.job.priority );
			$("#projectJobDeleteBtn").css("display", "inline-block");			
			
			hideLoading();
		}).fail(function(jqXHR, textStatus, errorThrown){
			hideLoading();
			alert('error: '+errorThrown);
		});
	}
}

//
// Jobs
//

function projectJobToggleState( jobId ) {
	showLoading();
	jobScroll = document.getElementById( getActivePane() + "_pane").scrollTop;
	
	$.ajax({
		type: "POST",
		url : "http://studio.wildmontage.com/app/project-toggle-job.php",
		data: {
			"project_id": Studio.Project.Id,
			"job_id": jobId
			},
		dataType : 'text',
		headers: {
		"Username":Studio.Username,
		"Password":Studio.Password
		}
	}).done(function(data){
		hideLoading();
		refreshInPane( getActivePane() );
	}).fail(function(jqXHR, textStatus, errorThrown){
		hideLoading();
		alert('error: '+errorThrown);
	});			
}

function projectJobsInit() {
	showLoading();
	
	$.ajax({
		type: "POST",
		url : "http://studio.wildmontage.com/app/project-get-task-and-jobs.php",
		data: {
			"project_id": Studio.Project.Id,
			"project_task": Studio.Project.Task
			},
		dataType : 'json',
		headers: {
		"Username":Studio.Username,
		"Password":Studio.Password
		}
	}).done(function(data){
		//Studio.Project.TaskName = data.task.name;
		
		//alert(data);
	
		$("#projectTaskName").text(data.task.name);
		$("#projectTaskDesc").text(data.task.desc);
		$("#projectTaskProg").text("("+data.task.progress+"% done, "+data.task.job_count+" jobs, "+data.task.jobs_done+" completed, "+(data.task.job_count-data.task.jobs_done)+" left.)");
		
		for (var job in data.jobs) {
			var jobId = data.jobs[job][0];
			var jobName = data.jobs[job][1];
			var jobDesc = data.jobs[job][2];
			var jobPriority = data.jobs[job][3];
			var jobDone = data.jobs[job][4];
			
			var imgThumb = document.createElement("IMG");
			imgThumb.src = "http://media.studio.wildmontage.com/projects/"+Studio.Project.Id+"/jobs/job_"+jobId+".jpg";
			imgThumb.style.maxWidth = "100%";
			
			
			var checkButton = document.createElement("BUTTON");
			checkButton.setAttribute("job-id", jobId);
			var checkI = document.createElement("I");
			checkI.className = "material-icons";
			if(jobDone==0){
				checkI.appendChild( document.createTextNode( "check_box_outline_blank" ) );
			}else{
				checkI.appendChild( document.createTextNode( "check_box" ) );
				checkI.style.color = "#00ff99";
			}
			checkButton.appendChild( checkI );
			checkButton.onclick = function(e) {
				e.stopPropagation();
				projectJobToggleState( this.getAttribute('job-id') );
				this.disabled = true;
			};
			
			var tRow = document.createElement("TR");
			
			var tThumbCol = document.createElement("TD"); tThumbCol.appendChild( imgThumb );	
			var tNameCol = document.createElement("TD"); tNameCol.appendChild( document.createTextNode( jobName ) );
			var tDescCol = document.createElement("TD"); tDescCol.appendChild( document.createTextNode( jobDesc ) );
			var tPriorityCol = document.createElement("TD"); tPriorityCol.appendChild( document.createTextNode( jobPriority ) );
			var tDoneCol = document.createElement("TD"); tDoneCol.appendChild( checkButton );
			
			tRow.appendChild(tThumbCol);
			tRow.appendChild(tNameCol);
			tRow.appendChild(tDescCol);
			tRow.appendChild(tPriorityCol);
			tRow.appendChild(tDoneCol);
			
			
			tRow.setAttribute("job-id", jobId);
			
			
			tRow.onclick = function() {
				jobScroll = document.getElementById( getActivePane() + "_pane").scrollTop;
				
				Studio.Project.Job = Number( this.getAttribute("job-id") );
				bEditingJob = true;
				openInPane('project_edit_job.html', 'tbProjects');
			};
			
			$("#jobTable").append( tRow );
		}
		
		document.getElementById( "tbProjects_pane").scrollTop = jobScroll;
		jobScroll=0;
		hideLoading();
		
	}).fail(function(jqXHR, textStatus, errorThrown){
		hideLoading();
		alert('error: '+errorThrown);
	});			
}