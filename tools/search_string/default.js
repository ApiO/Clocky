var sql_query = "select id, str from strings;";
function update_resources()
{	
	$("#update_resources").prop("disabled", true);
	if( $('#resources_table').dataTable.fnTables().length){
		 $('#resources_table').dataTable().fnDestroy();
	}	
	sql.open(data_path+"\\build.db", {}, function (err, db) {
		if (err) throw err;
		db.exec(sql_query, function(err, results) {
		
		   for (var i = 0; i < results.length; i++) {
		     results[i].id = results[i].id>>>0;
		   }
		
			if (err) throw err;
			$('#resources_table').dataTable({
				data: results,
				columns: [
					{ data: "id", title: "Id", width: "160px" },
					{ data: "str", title: "String" }]
			});
			$("#update_resources").prop("disabled", false);		
		});
	});
}