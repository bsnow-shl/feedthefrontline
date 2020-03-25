<?php

function dbadmin_list_tables() {
	return db_array(" SELECT c.relname FROM pg_catalog.pg_class c LEFT JOIN pg_catalog.pg_user u ON u.usesysid = c.relowner LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace WHERE c.relkind IN ('r','') AND n.nspname NOT IN ('pg_catalog', 'pg_toast') AND pg_catalog.pg_table_is_visible(c.oid) ORDER BY 1; ");
}

function dbadmin_list_sequences() {
    return db_array(" select c.relname from pg_catalog.pg_class c LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace WHERE c.relkind IN ('S','') AND n.nspname <> 'pg_catalog' AND n.nspname <> 'information_schema' AND n.nspname !~ '^pg_toast' AND pg_catalog.pg_table_is_visible(c.oid) order by 1");
}

function dbadmin_table_exists($table) {
    return in_array($table, dbadmin_list_tables());
}

function dbadmin_list_columns($table) {
	return db_multirow("
SELECT a.attname,
  pg_catalog.format_type(a.atttypid, a.atttypmod) as type,
    (SELECT substring(d.adsrc for 128) FROM pg_catalog.pg_attrdef d
	   WHERE d.adrelid = a.attrelid AND d.adnum = a.attnum AND a.atthasdef) as default_value,
	     a.attnotnull as notnull
		 FROM pg_catalog.pg_attribute a, pg_class c
		 WHERE a.attnum > 0 AND NOT a.attisdropped AND a.attrelid=c.oid AND relname=?
		 ORDER BY a.attnum
	", $table);
}

function dbadmin_column_exists($table, $column) {
	$cols = dbadmin_list_columns($table);
	foreach ($cols as $c) {
		if ($c['attname']==$column) {
			return true;
		}
	}
	return false;
}

function dbadmin_create_column($table, $column, $type, $indexed=false) {
	db_send("alter table $table add $column $type");
	if ($indexed) {
		$idxname = substr($table.'_'.$column.'_idx',0, 25);
		db_send("create index $idxname  on $table($column)");
	}
}

?>
