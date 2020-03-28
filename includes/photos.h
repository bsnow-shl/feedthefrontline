<?php
function photo_album_rescan($id) {
	set_time_limit(0);
	$scanned = 0;
	db_send('begin');
	db_send('set constraints all deferred');
	$directory = db_value('select album_uri from photo_albums where album_id=?', $id);
	db_send('update photos set small_width = null where album_id=?', $id);

	if (substr($directory, -1) != '/') {
		$directory .= '/';
	}
	$server_directory = $_SERVER['DOCUMENT_ROOT'].$directory;

	if (is_dir($server_directory)){ 
		$dh = opendir($server_directory);
		while (($file = readdir($dh)) !== false) {
			$files[] = $file;
		}
		closedir($dh);

		sort($files);

		foreach ($files as $file) {
			$medium_file = $server_directory.'medium/'.$file;
			$small_file = $server_directory.'small/'.$file;
			$large_file = $server_directory.$file;
			if (is_dir($large_file)) { continue; }
			$large_img = getimagesize($large_file);
			if (!$large_img || !$large_img[0]) { continue; }
			$medium_img = @getimagesize($medium_file);
			$small_img = getimagesize($small_file);

			$photo_id = db_value('select * from photos where album_id=? and large_uri=?', $id, $directory.$file);
			if (!$photo_id) { $photo_id = db_newid('photos'); }
			$large_md5 = trim(`md5sum $large_image | cut -c1-32`);
			db_insert_or_update('photos', 'photo_id', $photo_id, array(
				'album_id' => $id,
				large_uri => $directory.$file,
				large_width => $large_img[0],
				large_height => $large_img[1],
				large_md5 => $large_md5,
				medium_uri => $directory.'medium/'.$file,
				medium_width => $medium_img[0],
				medium_height => $medium_img[1],
				small_uri => $directory.'small/'.$file,
				small_width => $small_img[0],
				small_height => $small_img[1]
			));
			$scanned++;
		}
	}
	db_send('delete from photo_keywords where photo_id in (select photo_id from photos where small_width is null)');
	db_send('delete from photo_views where photo_id in (select photo_id from photos where small_width is null)');
	db_send('delete from photos where small_width is null');
	db_send('commit');
	return $scanned;
}
 

function photo($img, $align='right', $extra = array()) {
	# we accept a photo URI, number, or array
	$w = $h = $photo_id = 0;
	$uri = '';

	$size = $extra['size'];
	if (!$size) { $size = 'small'; }

	if (!is_array($img)) {
	# if it's numeric, grab its properties
	if ((string) intval($img)=== (string) $img) {
		$img = db_row('select * from photos where photo_id=?', $img);
	} else {
		# see if it's in the database
		$cachespot = '.photocache.txt';

		if (is_file($cachespot)) {
			eval(join('', file($cachespot)));
		}

		if (true || !$var[$uri]) {
			# see if there's a small version of it

			# first move into the current file's working dir
			$curfileinfo = pathinfo($_SERVER['SCRIPT_FILENAME']);
			chdir($curfileinfo['dirname']);

			# then check out the relative pathname
			$pi = pathinfo($img);
			$maybesmall = $pi['dirname'].'/small/'.$pi['basename'];
			if (file_exists($maybesmall)) {
				$uri = $maybesmall;
			} else {
				$uri = $img;
			}

			if (!$pi['dirname']) { $pi['dirname'] = '.'; }

			if (file_exists($uri)) {
				$details = getimagesize($uri);
				$var[$uri][width] = $details[0];
				$var[$uri][height] = $details[1];
			}

			$pi = pathinfo($_SERVER['PHP_SELF']);
			$large_uri = $pi[dirname].'/'.$img;
			$photo_id = $var[$uri][photo_id] = db_value('select photo_id from photos where large_uri=?', $large_uri); 
			$var[$uri]['caption'] = db_value('select  photo_title from photos where large_uri=?', $large_uri);

			$fd = fopen($cachespot, 'w');
			fwrite($fd, showvar($var));
			fclose($fd);
		}

		$w = $var[$uri][width];
		$h = $var[$uri][height];
		$caption = $var[$uri]['caption'];
		$photo_id = $var[$uri][photo_id];
		}
	}

	if (is_array($img)) {
		$w = $img[$size.'_width'];
		$h = $img[$size.'_height'];
		$uri = htmlentities($img[$size.'_uri']);
		$uri = str_replace("?", "%3f", $uri);
		$caption = $img['photo_title'];
		$photo_id = $img['photo_id'];
	}

	# display the image now that we have it
	$pre = $post = '';
	if ($align == 'left') {
		$pre = '<div class="svanimg" style="float: left; margin-right: 1em;">';
		$post = '</div>';
	} else if ($align == 'center') {
		$pre = '<center >';
		$post = '</center>';
	} else if ($align == 'right') {
		$pre = '<div  class="svanimg" style="float: right; margin-left: 1em;">';
		$post = '</div>';
	} else {
		$pre = '<table><tr><td class="svanimg">';
		$post = '</td></tr></table>';
	}

	if ($photo_id == $_GET['reload']) {
		$uriextra = "?time=".time();
	}

	if ($extra['alt']) {
		$alt = $extra['alt'];
	}
	$title = $extra['title'];

	echo $pre;
	echo "<a href=\"/photos/show/$photo_id\"><img class=\"svan\" alt=\"$alt\" title=\"$title\" src=\"$uri$uriextra\" width=\"$w\" height=\"$h\" /></a>";
	if (isset($extra['caption'])) {
		$render = $extra['caption'];
	} else {
		$render = $caption;
	}
	if ($render) {
		echo '<div class="caption">'.$render.'</div>';
	}
	echo $post;
}

function photos_make_thumbnails($id) {
	$directory = $_SERVER['DOCUMENT_ROOT'].'/'.db_value('select album_uri from photo_albums where album_id=?', $id);

	 if (!is_dir($directory)) {
	 	return null;
	}

	chdir ($directory);
	exec($_SERVER['DOCUMENT_ROOT'].'../bin/make-thumbnails', $ignored, $rc);
	if (file_exists($directory.'/.make-thumbnails-in-progress')) {
		echo "not ready yet";
		exit(0);
	}
	return $rc;
}


function photo_album_details($album, $extra_criteria = array()) {
	if ((string) intval($album) === $album) {
		$details = db_row('select * from photo_albums where album_id=?', $album);
	} else {
		$details = db_row('select * from photo_albums where album_uri like ?', '%/'.$album.'/');
	}
	$album_id = $details['album_id'];
	if (!$album_id) { return array(null,null); }

	# album id in hand, get to work
	$params = array($album_id);
	if ($extra_criteria['rating']) {
		$sql_extra .= ' and rating = ? ';
		$params [] = $extra_criteria['rating'];
	}

	$matching = db_multirow("select * from photos where album_id = ? $sql_extra order by album_order", $params);

	$csys = db_value("select comment_system_id from comment_systems where table_name='photos'");
	$comments = db_multirow("select commented_thing,comment,author_name,author_email,date_part('epoch', comment_timestamp) as comment_timestamp,user_id from comments where comment_system_id=$csys and commented_thing in (select photo_id from photos where album_id=$album_id)");
	foreach ($comments as $c) {
		$commentindex[$c['commented_thing']][] = $c;
	}

	$kw_sql = db_multirow('select photo_keywords.photo_id,keyword from photo_keywords ,photos where photo_keywords.photo_id = photos.photo_id and album_id=?', $album_id);
	foreach ($kw_sql as $kw=>$row) {
		$keywordindex[$row['photo_id']][] = $row['keyword'];
	}

	for ($i=0; $i<sizeof($matching); $i++) {
		$matching[$i]['keywords'] = $keywordindex[$matching[$i]['photo_id']];
		$matching[$i]['comments'] = $commentindex[$matching[$i]['photo_id']];
	}

	return array($details, $matching);
}

function photo_details($path_or_id) {
	if (is_numeric($path_or_id)) {
		$path = db_value('select small_uri from photos where photo_id=?', $path_or_id);
	} else {
		$path = $path_or_id;
	}

        $imagefilename = $_SERVER['DOCUMENT_ROOT'].'/'.$path;
        getimagesize($imagefilename, $info);
        if ($info['APP1']) {
                @eval (" \$dat = exif_read_data(\$imagefilename);");
        } else {
                $dat = array();
        }

	if ($dat['DateTimeOriginal']) {
		list ($date, $time) = preg_split('/ /', $dat['DateTimeOriginal']);
		list ($y,$m,$d) = preg_split('/:/', $date);
	}
        if ($y > 50) {
          list ($hr,$min,$sec) = split(':', $time);
          $time = mktime($hr,$min,$sec,$m,$d,$y);
        }

        $exposure = $dat[ExposureTime];
        if (!$exposure) {
                $exposure = $dat[COMPUTED][ExposureTime];
        }
        $camera_make = "$dat[Model]";
	$exposure = ($exposure);
	@eval(" \$focal = $dat[FocalLength]; ");
	$fstop = $dat['COMPUTED']['ApertureFNumber'];
	return array($time, $camera_make, $exposure, $fstop, $focal);
}


?>
