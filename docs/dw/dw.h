<?

function dw_get_facts($fact_id=0) {
	if ($fact_id) {
		$where = 'WHERE fact_id='.(0+$fact_id);
	}

	$rc = db_multirow("select fact_id, 'dwf_'||table_name as table_name, 'dwf_'||table_name||'_view' as view_name, pretty_name from dw_facts $where");
	return $rc;
}

function dw_get_fact_numbers_for($f) {
	return db_multirow('select fact_number_id, pretty_name, column_name, column_sql_type from dw_fact_numbers where fact_id=?', $f['fact_id']);
}

function dw_get_dimensions_for($t) {
	return db_multirow("select dimension_id, ? || '_' || table_name as table_name, ? || '_' || table_name || '_id' as dimension_pkey, pretty_name, loader_header, loader_function from dw_dimensions where fact_id=?", $t['table_name'], $t['table_name'], $t['fact_id']);
}

function dw_get_attributes_for($t) {
	$md5 = 'dwfi_'.substr(md5($t['table_name']), 0, 10).'_';

	return db_multirow("select column_name,column_sql_type, ? || column_name as index_name , ? || '.' || column_name as column_name_with_table, ? || '_' || column_name as view_column_name, pretty_name, is_identifying from dw_attributes where dimension_id=?", $md5, $t['table_name'], $t['table_name'], $t['dimension_id']);
}

function dw_get_kitchen_sink() {
	$facts = dw_get_facts();
	foreach ($facts as $f) {
		$fact_rc['fact'] = $f;
		$fact_rc['numbers'] = dw_get_fact_numbers_for($f);
		$dimensions = dw_get_dimensions_for($f);
		foreach ($dimensions as $d) {
			$fact_rc['dimensions'][] = array(
					'dimension' => $d,
					'attributes' => dw_get_attributes_for($d)
			);
		}
		$rc[] = $fact_rc;
	}
	return $rc;
}

function dw_create_tables() {
	set_time_limit(0); 
	ignore_user_abort(true);

	$facts = dw_get_facts();
	$statements[] = "drop sequence dwf_seq";
	$statements[] = "create sequence dwf_seq start 1";

	foreach ($facts as $ign=>$t) {
		$statements[] = 'drop view '.$t['view_name'];
		$statements[] = 'drop table '.$t['table_name'];

		$dimensions = dw_get_dimensions_for($t);
		$fact_ids = array();
		foreach ($dimensions as $d) {
			$attributes = dw_get_attributes_for($d);
			$dim_attr = array($d['dimension_pkey']." integer not null primary key default nextval ('dwf_seq')");

			$index_statements = $ident_attr = array();
			foreach ($attributes as $a) {
				$dim_attr[] = $a['column_name'].' '.$a['column_sql_type'];
				$index_statements[] = "create index ".$a['index_name'].' on '.$d['table_name'].'('.$a['column_name'].")";
				if ($a['is_identifying']) {
					$ident_attr[] = $a['column_name'];
				}
				$join_columns[] = $a['column_name_with_table'].' AS '.$a['view_column_name'];
			}
			
			$statements[] = "drop table ".$d['table_name'];
			$dimension_tables[] = $d['table_name'];
			$dimension_join[] = ' t.'.$d['dimension_pkey'].' = '.$d['table_name'].'.'.$d['dimension_pkey'];
			$statements[] .= "create table ".$d['table_name']." (\n\t".join(",\n\t",$dim_attr).",\n\tUNIQUE (".join(",", $ident_attr).")\n)";
			$statements = array_merge($statements, $index_statements);
			
			$fact_ids[] = $d['dimension_pkey']." integer not null references ".$d['table_name'];
			$dim_index =  '';
		}

		$numbers = dw_get_fact_numbers_for($t);

		foreach ($numbers as $n) {
			$fact_ids[] = $n['column_name'].' '.$n['column_sql_type'];
		}
	
		$fact_statements[] = "create table ".$t['table_name']." (\n\t".join(",\n\t", $fact_ids)."\n)";
		$view_statement = "create or replace view ".$t['view_name'].' AS select '.join(',', $join_columns).' FROM '.join(',',$dimension_tables).','.$t['table_name'].' as t WHERE '.join(' AND ',$dimension_join)."\n";
		$fact_statements[] = $view_statement;
	}
	
	$statements = array_merge($statements, $fact_statements);

	foreach ($statements as $s) {
		$result = @db_send($s);
		$err = db_errormessage();
		if ($err) {
			print '<blockquote><big>'.$err.'</big><br>during '.$s.'</blockquote><br/><br/>';
		} else {
		}
	}

	foreach ($facts as $ign => $t) {
		$dimensions = dw_get_dimensions_for($t);
		foreach ($dimensions as $d) {
			if (!$d['loader_header']) { continue; }

			print "Loading ".$d['pretty_name']."\n<br>";

			require_once $d['loader_header'];
			$d['loader_function']();
		}
	}
}


function dw_load_buyer_dimension() {
	$dbname= db_get('dbname');
	chdir($_SERVER['DOCUMENT_ROOT']);
	chdir('..');
	system("bin/load-table-in-bg.pl $dbname dwf_sales_buyer bin/dw-geo.sql.gz");
	print ".... loading in background.\n<br/>";
}

function dw_load_recipient_dimension() {
	$dbname= db_get('dbname');
	chdir($_SERVER['DOCUMENT_ROOT']);
	chdir('..');
	system("perl bin/load-table-in-bg.pl $dbname dwf_sales_recipient bin/dw-geo-sql.gz 85");
	print ".... loading in background.\n<br/>";
}

function dw_load_price_dimension() {
	$dbname= db_get('dbname');
	chdir($_SERVER['DOCUMENT_ROOT']);
	chdir('..');
	system("perl bin/dw-load-price.pl $dbname dwf_sales_price");
}

function dw_load_date_dimension() {
	print "...";
	$dbname= db_get('dbname');
	chdir($_SERVER['DOCUMENT_ROOT']);
	chdir('..');
	system("perl bin/dw-load-date.pl $dbname dwf_sales_date");

}


?>
