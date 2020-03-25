<?php
require_once 'i18n.h';


function whatsnew_get($whatsnew_id) {
	$home = config_get('site', 'home');
	$home = substr($home, 0, strlen($home)-1);

	$w = db_row('select * from whatsnew where whatsnew_id=?', $whatsnew_id);
	if (!$w) return FALSE;

	$extra_select='';
	if ($w['title_key'] && !$w['i18n_class']) {
		$extra_select =','.$w['title_key'].' AS whatsnew_title ';
	}
	if ($w['description_key'] && !$w['i18n_class']) {
		$extra_select .=','.$w['description_key'].' AS whatsnew_description ';
	}
	if ($w['date_key']) {
		$extra_select .= ','.$w['date_key'].' AS whatsnew_date ';
	}
	if ($w['author_key']) {
		$extra_select .= ", first_name || ' ' || last_name as author, email as author_email ";
		$extra_tables = " LEFT OUTER JOIN users as u ON u.user_id = ".$w['source_table'].'.'.$w['author_key'];;
	}
	$sql = 'SELECT *'.$extra_select.' from '.$w['source_table'].$extra_tables.' '.$w['where_clause'].$w['order_clause'];
	$result = db_multirow($sql);
	if ($w['i18n_class']) {
		i18n_get_batch($result, $w['i18n_class'], $w['source_table_pkey'], $w['i18n_language']);
		for ($i=0; $i<sizeof($result); $i++) {
			$result[$i]['whatsnew_title'] = $result[$i][ $w['title_key'] ];
			$result[$i]['whatsnew_description'] = $result[$i][ $w['description_key'] ];
		}
	}

	# prepare the whatsnew header
    $description = strip_tags($w['description']);
    if (strlen($description)>100) {
        $description = substr($description, strpos($description, ' ', 100)).'<br/><br/>...';
    }
	$struct[0] = array(
		'title' => $w['title'],
		'link' => $home.$w['uri'], 
		'description' => $description,
		'pubDate' => strftime("%a, %d %b %Y %T %z", time()),
		'lastBuildDate' => strftime("%a, %d %b %Y %T %z", strtotime_php5($result[0]['whatsnew_date'])),
		ttl => '60',
		generator => 'Content Publishing Framework http://tinyplanet.ca/cpf/'
	);

	# and the whatsnew body
	if ($w['link_header']) {
		require_once $w['link_header'];
	}

	foreach ($result as $r) {
		$link = '';
		if ($w['link_function']) {
			$link = $home.$w['link_function']($r);
		}
        $description = strip_tags($r['whatsnew_description']);
        if (strlen($description)>512) {
            $description = substr($description, 0, strpos($description, ' ', 512))." … (<a href=\"$link\">more</a>)";
        }
		$toadd = array(
			'title'=> $r['whatsnew_title'],
			'description' => $description,
			'link' => $link,
			'guid' => db_get('dbname').'/'.$w['source_table'].'/'.$r[$w['source_table_pkey']],
			'pubDate' => strftime("%a, %d %b %Y %T %z", strtotime_php5($r['whatsnew_date'])),
			'author' => $r['author'],
			'author_email' => $r['author_email']
			);
		$struct[] =  $toadd;
	}

	return $struct;
}

function whatsnew_rss ($struct) {
	if ($struct===false) return false;

	$hdr = array_shift($struct);
	$rss[] = '<?xml version="1.0"?>';
	$rss[] = '<rss version="0.91"><channel>';
	$rss[] = '<title>'.$hdr['title'].'</title>';
	$rss[] = '<link>'.$hdr['link'].'</link>';
	$rss[] = '<description>'.htmlspecialchars($hdr['description']).'</description>';
	$rss[] = '<ttl>'.$hdr['ttl'].'</ttl>';
	$rss[] = '<pubDate>'.$hdr['pubDate'].'</pubDate>';

	foreach ($struct as $ignored => $item) {
		$rss[] = '<item><title>'.htmlspecialchars($item['title']).'</title>';
		$rss[] = '<description>'.htmlspecialchars($item['description']).'</description>';
		$rss[] = '<guid isPermaLink="false">'.$item['guid'].'</guid>';
		$rss[] = '<link>'.$item['link'].'</link>';
		$rss[] = '<pubDate>'.$item['pubDate'].'</pubDate>';
		if ($item['author_email']) {
			$string = $item['author_email'];
			if ($item['author']) {
				$string .= ' ('.$item['author'].')';
			}
			$rss[] = '<author>'.$string.'</author>';
		}
		$rss[] = '</item>';


	}

	$rss[] = "</channel></rss>";

		
	return join("\n", $rss);

}

function whatsnew_adminmenu($which) {
	$id = db_value('select * from whatsnew where short_name=?', $which);
	if (!$id) return '' ;
	$rss = whatsnew_get($id);
	$header = array_shift($rss);
	print $header['title'];
	print '<br/>';
	foreach ($rss as $ignored => $item) {
		if ($count++>5) {
			last;
		}
		print '- <a href="'.$item['link'].'">'.$item['title'].'</a><br/>';
	}

	print '<br/>';
}
	
?>
