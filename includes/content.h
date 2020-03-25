<?
require_once 'i18n.h';
require_once 'cache.h';

function content_get_tree($language=null) {
	static $cached = null;
	if ($cached) return $cached;
	
	$query = content_get_info($language);

	foreach ($query as $q) {
		$rc[$q['parent_content_id']][] = $q;
	}
	return $cached = $rc;
}


function content_get_info($language=null) {
	static $cached = null;
	if ($cached) return $cached;
	
	$query = db_multirow("select i18n_content.content_id,date_part('epoch', create_tstamp) as create_tstamp, i18n_content_hierarchy.parent_content_id, i18n_content.short_name from i18n_content left outer join i18n_content_hierarchy using (content_id) order by display_order");
	i18n_get_batch($query, 'Content item', 'content_id', $language);
	foreach ($query as $q) {
		$rc[ $q['content_id'] ] = $q;
	}
	return $cached = $rc;	
}


function content_dump_ul_tree($cid, $id_list = array(), $depth=2) {
        if ($cid==0) { return; }
	if ($depth==0) { return; }

	$tree = content_get_tree(); 
	$info = content_get_info();
        global $counter;
	$id = array_shift($id_list);
        if (is_array($tree[$cid])) {
                echo "<ul ".($id ? "id=\"$id\"" : ''). " class=\"$id\" >";
                foreach ($tree[$cid] as $i) {
			if (substr($info[$i['content_id']]['title'],0,1) == '.') { continue; }
                        $daddy = $tree[ $i['content_id'] ] ? 1 : 0;
                        printf ('<li><a %s href="/content/%s">%s</a>', $daddy ? 'class="daddy"' : '', $info[$i['content_id']]['short_name'],$info[$i['content_id']]['title']);
                        if ($info[ $i['content_id'] ]) {
                                content_dump_ul_tree($i['content_id'], $id_list, $depth-1);
                        }
                        echo '</li>';
                }
                echo '</ul>';
        }

}

function content_user_require($content_row, $enforce=true) {
    require_once 'users.h';
    if ($enforce) {
        user_require(); 
    } else {
        return user_id() > 0;
    }
}
?>
