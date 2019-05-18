var sql_query = "SELECT types.name AS type, (strings.str||'.'||types.ext) AS name, data.file AS data_file FROM data INNER JOIN types on (data.type == types.id) INNER JOIN strings on (data.name == strings.id);";
function update_resources()
{	
	$("#update_resources").prop("disabled", true);
	if( $('#resources_table').dataTable.fnTables().length){
		 $('#resources_table').dataTable().fnDestroy();
	}	
	sql.open(data_path+"\\build.db", {}, function (err, db) {
		if (err) throw err;
		db.exec(sql_query, function(err, results) {
			if (err) throw err;
			$('#resources_table').dataTable({
				data: results,
				columns: [
					{ data: "type", title: "Type", width: "150px" },
					{ data: "name", title: "Name" },
					{ data: "data_file", title: "Data", width: "120px" }]
			});
			$("#update_resources").prop("disabled", false);
			
			$('#resources_table').dataTable().$('tr').dblclick( function () {
				var data = $('#resources_table').dataTable().fnGetData(this);
				open(source_path+"\\"+data.name);
			} );			
		});
	});
}