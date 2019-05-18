var list_view = null;

function add_issue(issue)
{	
	var link = document.createElement('a');
	link.className = "list-item";
	link.href = '#' + issue.res;
	$(link).dblclick( function () {
		open(source_path+"\\"+ this.href.substring(this.href.indexOf('#')+1));
	});
	
	var content = document.createElement('div');
	content.className = "list-content";
	
	var span_title = document.createElement('span');
	span_title.innerHTML = "Resource: " + issue.res;
	span_title.className = "list-title";
	
	var span_subtitle = document.createElement('span');	
	span_subtitle.innerHTML = "Missing " +(issue.isRef ? "reference." : "dependency.");
	span_subtitle.className = "list-subtitle "+(issue.isRef ? "ref" : "dep");
	
	var span_description = document.createElement('span');	
	span_description.innerHTML = issue.lnk;
	span_description.className = "list-description";
	
	content.appendChild(span_title);
	content.appendChild(span_subtitle);
	content.appendChild(span_description);
	link.appendChild(content);
	list_view.appendChild(link)
}

function clear_list()
{
	while (list_view.firstChild) {
		list_view.removeChild(list_view.firstChild);
	}
}

var sql_query = "SELECT (s1.str ||'.'|| t1.ext) AS res, (s2.str ||'.'|| t2.ext) AS lnk, vue.isRef AS isRef FROM ( SELECT vue.res_type, vue.res_name, vue.link_type, vue.link_name, vue.isRef FROM ( SELECT d1.name AS res_name, d1.type AS res_type, d1.dep_name as link_name, d1.dep_type as link_type, 0 as isRef FROM deps AS d1 WHERE d1.dep_type != 0 GROUP BY d1.name, d1.type, d1.dep_name, d1.dep_type UNION ALL SELECT d1.name AS res_name, d1.type AS res_type, d1.ref_name as link_name, d1.ref_type as link_type, 1 as isRef FROM refs AS d1 WHERE d1.ref_type != 0 GROUP BY d1.name, d1.type, d1.ref_name, d1.ref_type ) AS vue LEFT JOIN data ON (vue.link_name = data.name AND vue.link_type = data.type) WHERE data.name is NULL ) AS vue INNER JOIN types AS t1 on (vue.res_type == t1.id) INNER JOIN types AS t2 on (vue.link_type == t2.id) INNER JOIN strings AS s1 on (vue.res_name == s1.id) INNER JOIN strings AS s2 on (vue.link_name == s2.id);";
function update_issues()
{ 
	$("#update_issues").prop("disabled", true);
    clear_list();
	sql.open(data_path+"\\build.db", {}, function (err, db) {
		if (err) throw err;
		db.exec(sql_query, function(err, results) {
			if (err) throw err;
			if(results.length == 0) {
				var span = document.createElement('span');
				span.innerHTML = "no error.";
				list_view.appendChild(span);
			}
			else
			{
			console.log(results);
				for (var i in results) {
					add_issue(results[i]);
				}
			}
		});
	});
	$("#update_issues").prop("disabled", false);
}