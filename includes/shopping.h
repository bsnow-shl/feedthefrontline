<?php

require_once 'i18n.h';
require_once 'users.h';
require_once 'cache.h';


/** tools for rendering the front-end or various admin screens */

function shopping_get_tree() {
	return shopping_get_tree_real();
}

function shopping_get_tree_menu($cat_id) {
	$tree = shopping_get_tree();

	$rc = array();
	if (!$tree[$cat_id]) return null;
	
	if ($tree[$cat_id]) foreach ($tree[$cat_id] as $t) {
		$cid = $t['category_id'];
		if ($tree[$cid]) {
			$rc[$cid] = array('type'=>'menu', 'label' => $t['short_name'], 'menu'=>shopping_get_tree_menu($cid));
		}  else {
			$rc[$cid] = beautify_identifier($t['short_name']);
		}
	}
	return $rc;
}
function shopping_get_tree_checkboxes($cat_id) {
	$tree = shopping_get_tree();

	$rc = array();
	if (!$tree[$cat_id]) return null;
	
	if ($tree[$cat_id]) foreach ($tree[$cat_id] as $t) {
		$cid = $t['category_id'];
		if ($tree[$cid]) {
			$interim = shopping_get_tree_checkboxes($cid);
			foreach ($interim as $k=>$v) {
				$rc[$k] = array( 'type'=>'flag', 'pretty' => beautify_identifier($t['short_name']).' - '.$v['pretty']);
			}
		}  else {
			$rc[$cid] = array('type'=>'flag', 'pretty'=>beautify_identifier($t['short_name']));
		}
	}
	return $rc;
}

function shopping_get_info() {
	static $cached = null;
	if ($cached) return $cached;
	
	$query = db_multirow("select shopping_categories.category_id, short_name, parent_category_id from 
				shopping_categories left outer join shopping_category_category_map using (category_id) order by display_order");
	foreach ($query as $q) {
		$rc[ $q['category_id'] ] = $q;
	}
	return $cached = $rc;	
}

function shopping_get_tree_real() {
	$query = shopping_get_info();

	foreach ($query as $q) {
		$rc[$q['parent_category_id']][] = $q;
	}
	return $rc;
}

/** Returns a list of the products in the category, given the category ID or short_name.
 * title
 * sku
 * price
 * options
 */
function shopping_get_category($cid, $category_name='', $realm_id='') {
	if (!$cid) {
		$cid = db_value('select category_id from shopping_categories where short_name=?', $category_name);
	}
	if (!$cid) { redirect('/'); }

	$cat = db_row('select * from shopping_categories where category_id=? ', $cid)+
	i18n_get('Product category', $cid);

	if ($realm_id) {
		$contents = db_multirow('select * from shopping_product_category_map join shopping_products using (product_id) join shopping_realm_product_map using (product_id) where visible_public=1 and category_id=? and realm_id=? order by display_order', $cid, $realm_id);
	} else {
		$contents = db_multirow('select * from shopping_product_category_map join shopping_products using (product_id) where visible_public=1 and category_id=? order by display_order', $cid);
	} 
	i18n_get_batch($contents, 'Product', 'product_id');

	return array($cat, $contents);
}



/** Returns the list of product details for a given sku or product ID.
	- price
	- tax_scheme_id
	- supplier_id
	- product_type_id
	- options - a list of options, providing:
		-- option_name
		-- option_type
		-- option pretty name
		-- menu (if type=menu) containing:
		--- key/value pairs of type option choice id => name/pretty/pricepremium array
	- former_price
	- former_price_reason
	- buyable boolean
	- special_notice
*/
function shopping_get_product($sku, $product_id=0, $language=null) {
	if (!$language) { $language = CPF_LANGUAGE; }
	if ($product_id) {
		$rc = db_row('select shopping_products.*,shopping_product_types.class_id from shopping_products join shopping_product_types using(product_type_id) where product_id=? and visible_public=1', $product_id);
	} else {
		$rc = db_row('select shopping_products.*,shopping_product_types.class_id from shopping_products join shopping_product_types using (product_type_id) where sku=? and visible_public=1', $sku);
	}
	if (!$rc) { return array(); }

	$rc['buyable'] = true;
	$rc['options'] = shopping_get_product_options($rc['product_type_id']);
	$rc['type'] = i18n_get('Product type', $rc['product_type_id'], $language);
	if ($rc['class_id']) {
			$rc += i18n_get($rc['class_id'], $rc['product_id'], $language);
	}
	$rc += i18n_get('Product', $rc['product_id'], $language);
	return $rc;
}

function shopping_get_product_options($product_type_id) {
	$opts = db_multirow('select * from shopping_product_type_options left outer join shopping_product_type_option_choices using (type_option_id) where product_type_id=?', $product_type_id);
	if (!$opts) { return array(); }

	i18n_get_batch($opts, 'Product type option', 'type_option_id');
	/* todo paste in the menu options */
	return $opts;
}


/** Renders HTML for a product image. */
function shopping_product_image($sku, $size='S') {
	if (strpos($sku,'/')!== FALSE) {
		$sku = "sku".db_value('select product_id from shopping_products where sku=?', $sku);
	} 

	echo '<img border="0" src="/products/images/'.$sku."-$size.jpg\" class=\"productimage\" />";
}





/* ---- shopcart management -- */


/** Get the strong shopcart ID.
 */
function shopping_shopcart_id() {
	static $answer = null;
	if (!$answer) {
		$answer = $_SESSION['shopcart_id'];
		$status = db_value('select status from shopping_shopcarts where shopcart_id=?', $answer);
		if ($status != 'new' && $status != 'finalized') $answer = null;	
	}
	return $answer;
}

function shopping_strong_shopcart_id() {
	return db_value('select strong_shopcart_id from shopping_shopcarts where shopcart_id=?', shopping_shopcart_id());
}

/** Adds a product to the shopcart with the given option list.
	@return shopcart entry id
	*/
function shopping_add_product_to_shopcart($product_id, $type_option_list=array() ){
	  $sid = shopping_shopcart_id();
		db_send('begin');
		if (!$sid || db_value('select count(*) from shopping_shopcarts where shopcart_id=? and status=?', $sid, 'new')==0) {
			# new shopcart or the old one is toast: make a new one
			$sid = $_SESSION['shopcart_id'] = db_newid('shopping_shopcarts');

			$strong_sid = '';
			while (!$strong_sid || db_value('select count(*) from shopping_shopcarts where strong_shopcart_id=?', $strong_sid)>0) {
				$strong_sid = substr(md5(uniqid(rand())), 0, 32);
			}

			db_send('insert into shopping_shopcarts (shopcart_id,strong_shopcart_id,session_id, user_id) values(?,?,?,?)', $sid, $strong_sid, session_id(), user_id());
		}
	
		db_send('insert into shopping_shopcart_entries(shopcart_entry_id, product_id, shopcart_id)
						values (?,?,?)', $seid=db_newid('shopping_shopcart_entries'), $product_id, $sid);

		$options = shopping_get_product_options(db_value('select product_type_id from shopping_products where product_id=?', $product_id) );

		foreach ($options as $o) {
			if ($type_option_list[$o['option_name']]) {
				db_send('insert into shopping_shopcart_entry_options(shopcart_entry_id,type_option_id,value) values (?,?,?)', 
							$seid, $o['type_option_id'], $type_option_list[$o['option_name']]);
			}
		}
		shopping_add_diary('Added product #'.$product_id.', creating shopcart entry #'.$seid);
		db_send('commit');
		return $seid;
}

function shopping_set_shopcart_status($sid, $status) {
  $oldstatus = db_value('select status from shopping_shopcarts where shopcart_id = ?',$sid);
  db_send('begin');
  db_update('shopping_shopcarts','shopcart_id',$sid,array('status'=>$status));  
  shopping_add_diary('Updated shopcart '.$sid.' status from: '.$oldstatus.' to '.$status, $sid);
  db_send('commit');
}

/** Counts how many items are in a shopcart.
 */
function shopping_count_shopcart_items() {
	$sid = shopping_shopcart_id();
	if (!$sid) { return 0; }

	return db_value('select count(*) from shopping_shopcart_entries where dropped=0 and shopcart_id=?', $sid);
}



/* Returns two things:
 * 1. a list of the entries in the shopping cart
 *    -- sku
 *    -- price
 *    -- title
 *    -- etc., see shopping_get_product
 * 2. a list of facts about the shopping car
 * 		-- 'total'
 */
function shopping_get_shopcart_contents($strong_shopcart_id=0, $language=null) {
	if (!$language) { $language = CPF_LANGUAGE; }
	if ($strong_shopcart_id) {
		$sid = db_value('select shopcart_id from shopping_shopcarts 
											where strong_shopcart_id=?', $strong_shopcart_id);
	} else  {
		$sid = shopping_shopcart_id();
	}
	if (!$sid) { return array(FALSE,FALSE); }

	$rc = db_multirow('select * from shopping_shopcart_entries where dropped=0 and shopcart_id=?', $sid);
	$facts = db_row('select * from shopping_shopcarts where shopcart_id=?', $sid);
	$entry_options = db_multirow("select shopcart_entry_id, product_id, option_name, value FROM shopping_shopcart_entries join shopping_shopcart_entry_options using (shopcart_entry_id) join shopping_product_type_options using (type_option_id) WHERE shopcart_id=?", $sid);
	for ($i=0; $i<sizeof($entry_options); $i++) {
	  //$entry_option_index[$entry_options[$i]['product_id']][$entry_options[$i]['option_name']] = &$entry_options[$i]['value'];
	  $entry_option_index[$entry_options[$i]['shopcart_entry_id']][$entry_options[$i]['option_name']] = &$entry_options[$i]['value'];
	}

	for ($i=0; $i<sizeof($rc); $i++) {
		$rc[$i] += cache_memoize('shopping_get_product', '', $rc[$i]['product_id'], $language);
		$rc[$i]['option_values'] = &$entry_option_index[$rc[$i]['shopcart_entry_id']];
		$facts['total'] += $rc[$i]['price'];

		if ($rc[$i]['address_id']) {
			$recip_row = db_row('select user_id, session_id, nickname, address_services.pretty_name from user_addresses left outer join address_services using (address_service_id)  where address_id=?', $rc[$i]['address_id']);
			if ($recip_row['user_id'] && $recip_row['user_id'] == user_id()) {
				exit('not yet implemented');
		  } elseif ($recip_row['session_id']) {
				# anonymous
				$rc[$i]['recipient']['pretty_description'] = $recip_row['nickname'];
				$rc[$i]['recipient']['via_description'] = $recip_row['pretty_name'];
		  } else {
				exit();
			}
	  }
	}
	if ($facts['cached_price'] != $facts['total']) {
		db_send('update shopping_shopcarts set cached_price=? where shopcart_id=?', $facts['total'], $sid);
		$facts['cached_price'] = $facts['total'];
  }
	return array($rc, $facts);
}




/** Drops a given shopcart entry id. */
function shopping_drop_shopcart_entry($shopcart_entry_id) {
	$sc = db_row('select shopping_shopcarts.shopcart_id,status from shopping_shopcart_entries join shopping_shopcarts using (shopcart_id)  where shopcart_entry_id=?', $shopcart_entry_id);
	if ($sc['status'] != 'new' || $sc['shopcart_id'] != shopping_shopcart_id()) { return "Shopcart ID mismatch"; }

	shopping_add_diary('Dropped entry #'.$shopcart_entry_id);
	db_send('update shopping_shopcart_entries set dropped=1 where shopcart_entry_id=?', $shopcart_entry_id);
}




/** updates the recipient for a shopcart entry
 * shopcart_entry_id -- which item we're talking about
 * address_id -- the address_id of the person getting the gift
*/
function shopping_set_recipient($shopcart_entry_id, $address_id) {
  # ensure the shopcart in question is the current one
	$sc = db_row('select shopping_shopcarts.shopcart_id,status from shopping_shopcart_entries join shopping_shopcarts using (shopcart_id)  where shopcart_entry_id=?', $shopcart_entry_id);
	if ($sc['status'] != 'new' || $sc['shopcart_id'] != shopping_shopcart_id()) { return "Shopcart ID mismatch"; }

	shopping_add_diary('Set recipient to address id #'.$address_id.' for entry id #'.$shopcart_entry_id);
	db_send('update shopping_shopcart_entries set address_id=? where shopcart_entry_id=?', $address_id, $shopcart_entry_id);
}



/** Adds a diary note to a shopcart. 
 */
function shopping_add_diary($note, $shopcart_id=0) {
	if (!$shopcart_id) { $shopcart_id = shopping_shopcart_id(); }
	
	if ($shopcart_id) {
		return db_send('insert into shopping_shopcart_diary(shopcart_id, user_id, message) values (?,?,?)', $shopcart_id, user_id(), $note);
	}
	return null;
}




/** Finalizes a shopcart with the given details. 
 * details {
 * 	name
 * 	phone
 * 	email
 * 	user_id
 * 	company
 * 	address
 * 	address_2
 * 	city
 * 	province
 * 	country
 * }
 *
 * Returns an English reason why not, or '' if it worked.
 **/
function shopping_finalize_shopcart($details) {
	$sid = shopping_shopcart_id();

	$current_status = db_value('select status from shopping_shopcarts where shopcart_id=?', $sid);
	if ($current_status != 'new') {
		shopping_add_diary("Tried to finalize again while status is $current_status.", $sid);
		if ($current_status == 'finalized') {
			# do nothing
		} else {
			return "Your order is already being processed.";
		}
	} else {
		shopping_add_diary('Finalized shopcart.');

		$update = array(
			'status'=>'finalized',
		);
		if ($details['email']) $update['buyer_email'] = $details['email'];
		if ($details['name']) $update['buyer_name'] = $details['name'];
		if ($details['phone']) $update['buyer_phone'] = $details['phone'];
		if ($details['user_id']) $update['user_id'] = $details['user_id'];
		if ($details['language_id']) $update['language_id'] = $details['language_id'];
		if ($details['company']) $update['buyer_company'] = $details['company'];
		if ($details['use_card_on_file']) $update['buyer_use_card_on_file'] = $details['use_card_on_file'];
		if ($details['address']) $update['buyer_address'] = $details['address'];
		if ($details['address_2']) $update['buyer_address_2'] = $details['address_2'];
		if ($details['city']) $update['buyer_city'] = $details['city'];
		if ($details['province']) $update['buyer_province'] = $details['province'];
		if ($details['country']) $update['buyer_country'] = $details['country'];
		if ($details['postal_code']) $update['buyer_postal_code'] = $details['postal_code'];
		if ($details['cell']) $update['buyer_cell'] = $details['cell'];
		db_update('shopping_shopcarts', 'shopcart_id', $sid, $update);
		return '';
	}
}


/** Administrative: creates product images.
 **/
function shopping_make_product_images($sku, $old_file, $size='') {
	$new_file = $_SERVER['DOCUMENT_ROOT'].'products/images/'.$sku;

	if (!$size || $size=='S') {
		$geo = config_get('shopping', 'small_geometry');
		if (!$geo) { $geo = '150x150';   }
		system("convert -geometry $geo $old_file  $new_file-S.jpg");
	}

	if (!$size || $size=='M') {
		$geo = config_get('shopping', 'medium_geometry');
		if (!$geo) { $geo = '330x220';  }
		system("convert $old_file -geometry $geo  -colorspace RGB $new_file-M.jpg");
	}

	if (!$size || $size=='L') {
		system("cp $old_file  $new_file-L.jpg");
	}
}

function shopping_clear_shopcart() {
		unset($_SESSION['shopcart_id']);
		unset($_SESSION['shopping']);
}


/* Here, we got a notification on a paypal invoice that someone has paid or gotten refunded. */
function shopping_paypal_notify($notify_id, $invoice_id, $paid_or_refunded) {
	$problem = 0;

	# get out the shopcart ID
	$sid = db_value('select system_reference from paypal_transactions where paypal_transaction_id=?', $invoice_id);
	if (!$sid) { $problem++; }

	if ($paid_or_refunded < 0) { # they got refunded
		$problem++;

		$r = db_send('update shopping_shopcarts set paid=0 where shopcart_id=?', $sid);
		if (1 != db_tuples($r)) { $problem++; }
		$r = shopping_add_diary('PayPal notification: customer was refunded', $sid);
		if (1 != db_tuples($r)) { $problem++; }
	} else {
		# they paid
		# make sure they didn't pay already
		$old_paid = db_value('select paid from shopping_shopcarts where shopcart_id=?', $sid);

		if ($old_paid) {
			shopping_add_diary('PayPal notification:  customer already paid for shopcart', $sid);
			$problem++;
		}
		
		$r = db_send('update shopping_shopcarts set paid=1 where shopcart_id=?', $sid);
		if (1 != db_tuples($r)) { $problem++; }
		$r = shopping_add_diary('PayPal notification:  customer paid', $sid);
		if (1 != db_tuples($r)) { $problem++; }
	}
	if ($problem) { db_send('update shopping_shopcarts set needs_attention=1 where shopcart_id=?', $sid); }

	return $problem;
}


function shopping_base($realm_id=0) {
	if (!$realm_id) {
		$realm_id = $_SERVER['shopping_realm'];
	}
	if ($realm_id) { return db_value('select uri from shopping_realms where realm_id=?', $realm_id); }
	return '';
}

?>
