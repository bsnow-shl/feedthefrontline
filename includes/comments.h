<?php

function comments_on($what, $which, $style='inline', $include_link=true) {
    $csys = db_row('select * from comment_systems where table_name=?', $what);
    if (!$csys) { return; }
    
    if ($style == 'inline') {
	echo '<div class="readercomments">';
        echo '<h6><b>Reader Comments</b></h6>';

        $comments = db_multirow("select *,date_part('epoch', comment_timestamp) as comment_timestamp  from comments left outer join users using(user_id) where comment_system_id=? and commented_thing=? order by comments.comment_timestamp", $csys['comment_system_id'], $which);
        foreach ($comments as $c) {
            echo format_comment($c);
        }
        if ($include_link) {
            if ($_SERVER['PHP_SELF'] != '/comments/index.html') {
                echo '<a href="'.href('/comments/','what', $what, 'which', $which, 'goback', $_SERVER['REQUEST_URI']).'">Add a comment...</a>';
            }
        }
	echo '</div>';
    } elseif ($style == 'brief') {
        $c = db_value('select count(*) from comments where comment_system_id=? and commented_thing=?', $csys['comment_system_id'], $which);
        echo '-- <a href="'.href('/comments/','what', $what, 'which', $which, 'goback', $_SERVER['REQUEST_URI'],'comments', $c, 'verbose',1).'">'.($c==1 ? '1 comment':"$c comments").'</a>.';
    } 
}

function format_comment($c) {
	$date = date('l, F j Y', $c['comment_timestamp']);

	if ($c['user_id']) {
        return sprintf('<p><span class="unimportant"><b>%s</b> on %s</span><br/>%s</p>', $c['first_name'], date("d F Y", $c['comment_timestamp']),  nl2br($c['comment']));
	} else {
		if (!$c['author_name']) {
			$c['author_name'] = $c['author_email'];
		}
		if (!$c['author_name']) {
			$c['author_name'] = 'anonymous';
		}

        return sprintf('<p class="unimportant"><b>%s</b> on %s</p><p>%s</p>', $c['author_name'], date("d F Y", $c['comment_timestamp']),  nl2br($c['comment']));
	}
}

?>
