<?

function geo_countries_menu($lang_id='en') {
	  $lang_id = substr($lang_id, 0, 2);

    static $_rc;
    global $Gconfig;
    if (!$_rc) {
        $contents = db_multirow('select iso_code as k, name_'.$lang_id.' as v,display_order from countries where visible=1 and display_order is not null
                union all
                                select iso_code as k, name_'.$lang_id.' as v,null as display_order from countries  where visible=1
                order by display_order,v
                                ');
        for ($i=0; $i<sizeof($contents); $i++) {
            $menu[$contents[$i]['k']] = $contents[$i]['v'];
        }
        $rc = array('type' => 'menu', 'menu' => $menu);
        if ($Gconfig['geo']) {
            $rc['default'] = $Gconfig['geo']['favourite_country'];
        }
        $_rc = $rc;
    }
    return $_rc;

}

function geo_province_list_for_country($country='', $lang_id='en') {
    $lang_id = substr($lang_id, 0, 2);
    if ($country) {
        $provinces = db_multirow('select * from country_provinces where country_code=? order by order_in_country', $country);
    } else {
        $provinces = db_multirow('select * from country_provinces order by country_code,order_in_country', $country);
    }
    $menu = array();
    $toget = 'name_'.$lang_id;

    foreach ($provinces as $p) {
        $menu[ $p[province_code]] =  $p[$toget];
    }
    return $menu;
}


function geo_province_menu_for_country($country='', $extra=array(),$lang_id='en') {
	$menu = geo_province_list_for_country($country, $lang_id);
    return array('type' => 'menu', 'menu' => $menu)+ $extra;
}

function geo_country_name($country, $lang_id='en') {
	return db_value("select name_$lang_id from countries where iso_code=?", $country);
}

function geo_form_for_country($country, $lang_id='en') {
	$basic = array(
	  'country' => array('type'=>'visible', 'value'=>db_value("select name_$lang_id from countries where iso_code=?", $country)), 
		'street' => array('type'=>'text','pretty'=>'Street address'),
		'apartment' => 'text',
		'city' => 'text'
	);

	$menu = geo_province_list_for_country($country, $lang_id);

	if ($menu) { 
		$name = 'Province';
		if ($country == 'US') {
			$name = 'State';
		}
		$basic['province'] = array('type'=>'menu', 'menu'=>$menu, 'pretty'=>$name);
	} else {
    $basic['province'] = 'text';
  }

	$basic['postal_code'] = array('type'=>'text', 'pretty'=> $country == 'US' ? 'Zip code' : 'Postal code');
	return $basic;

}

?>
