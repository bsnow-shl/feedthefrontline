<?
require_once 'users.h';

global $weblog_id;

$url_parts = split('/', $_SERVER['PHP_SELF']);
$weblog_name = $url_parts[2];
$weblog_id = db_value('select weblog_id from weblogs where short_name=?', $weblog_name);

# returns nonzero if the current user is the weblog's nuthor
function weblog_is_author($id='') {
	global $weblog_id;
	if ($id) {
		$weblog_id = $id;
	}
	return db_value("select count(*) from weblogs where weblog_id=? and weblog_author=?", $weblog_id, user_id());
}

?>
