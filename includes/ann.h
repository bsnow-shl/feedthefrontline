<?php 

include_once 'i18n.h';

# include_past = true to include events dated in the past
# include_future = true to include events that haven't been posted yet
function ann_get($include_past = false, $include_future=false, $category=null) {
	$cond = "true";

	if ($include_past) {
	} else {
		$cond .= ' and (takedown_date is null or (takedown_date is not null and takedown_date> CURRENT_TIMESTAMP)) ';
	}

	if (!$include_future) {
		$cond .= ' and posting_date < CURRENT_TIMESTAMP ';
	}

	if (!is_null($category)) {
		$cond .= ' and category_id=? ';
		$params[] = $category;
	}

	$rc = db_multirow("select *,date_part('epoch', start_date) as startdate, date_part('epoch', end_date) as enddate  from announcements where ".$cond.' order by start_date desc', $params);
	i18n_get_batch($rc, 'announcement', 'announcement_id');
	$rc2 = array();
	foreach ($rc as $r) {
		if ($r['title']) {
			$rc2[] = $r;
		}
	}
	return $rc2;
}


# use this to fetch the particulars of a given announcement
function ann_fetch($id) {
	return db_row("select *,date_part('epoch', start_date) as startdate, date_part('epoch', end_date) as enddate  from announcements
			                        where announcement_id=?", $id);
}


function ann_render($ann, $style="summary", $lang_id='') {
	if (!$lang_id) {
		$lang_id = CPF_LANGUAGE;
	}

	$title = $ann['title'];
	$caption = $ann['caption'];
	$body = $ann['body'];

	# configure for locale
	if ($lang_id=='fr') {
		setlocale(LC_TIME,'fr_CA');
		$fmt = "Le %e %B %Y";
	} else {
		setlocale(LC_TIME,'en_CA');
		$fmt = "%B %e, %Y";
	}


	echo '<div class="announcement">';
	$start_date = strftime($fmt, $ann['startdate']);
	if ($ann['enddate']) {
		$end_date = strftime($fmt, $ann['enddate']);
		$date = "$start_date - $end_date";
	} else {
		$date = "$start_date";
	}

	$url = $ann['url'];
	if (!$url && $ann['upload_id']) {
		$fname = db_value('select original_name from uploads where upload_id=?', $ann['upload_id']);
		$url = '/ann/special.html/'.$ann['upload_id'].'/'.urlencode($fname);
	}

	if ($style == 'full') {
		echo '<h3 class="title">'.$title.'</h3>';
		if ($date) {
			echo '<div class="announcedate">'.$date.'</div>';
		}
		echo '<div class="caption">';
		echo $caption;
		echo '</div>';
		if ($style = 'full') {
			echo '<div class="body">'. $body .'</div>';
		}
		echo '</div>';
	} else {
		if (!$url) {
			$url = '/ann/show.html/'.$ann['announcement_id'];
		}

		echo '<div class="title"><a href="'.$url.'">'.$date.': '.$title.'</a></div>';
		if ($style != 'headlineonly') {
			echo '<div class="caption">'.$caption.'</div>';
		}
	}

	echo '</div>';
	setlocale(LC_TIME, 'C');
}

function whatsnew_link($struct) {
	return '/ann/show.html/'.$struct['announcement_id'];
}

?>
