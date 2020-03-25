<?php
db_send('rollback');

global $_Template;
$_Template->print_footer();

if (!$_Template->template_is_disabled() && ($_SERVER['sqldebug'] || $_REQUEST['sqldebug'])) {
	if ($_sql_debug) { 
		for ($i=0; $i<sizeof($_sql_debug); $i++) {
			$sql_time += $_sql_debug[$i]['time'];
			$sql_rows += $_sql_debug[$i]['rows'];
		}
		printf ("%.0f ms to get %d rows from SQL</p>", $sql_time*1000, $sql_rows);
		if ($_REQUEST['sqldebug']!='brief' && ($_REQUEST['sqldebug']=='detailed' || $_GET['sqldebug'])) {
			print_a($_sql_debug); 
		}
	}
}
if ($_REQUEST['phpprofile']) {
	global $_xdebug_trace_file;

	echo '<pre>';
	xdebug_dump_function_profile(6);
	xdebug_stop_trace();
	echo '</pre>';
}
?>

