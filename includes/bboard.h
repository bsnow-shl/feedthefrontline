<?php

function bboard_whatsnew_link($what) {
	$msgid = $what['bboard_message_id'];
	if ($what['parent_post']) {
		$msgid = $what['parent_post'];
	}

	$short = db_value('select short_name from bboards where bboard_id=?', $what['bboard_id']);
	return "/bboard/$short/$msgid";

}

function bboard_config($shortname) {
  $parameter = config_get('bboard','needs_approval');
  if ($parameter == 'all' 
      || (is_array($parameter) && in_array($shortname,$parameter)) ) 
    return "approval";
  else
    return "standard";
}

?>
