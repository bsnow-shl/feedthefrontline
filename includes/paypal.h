<?php

function paypal_add_diary_note($txnid, $note) {
	db_send('insert into paypal_transaction_diary(paypal_transaction_id, note) values (?,?)', $txnid, $note);
}

# extra keys
# -- newtitle
function paypal_redirect($system_id, $item_id, $price, $extra=array()) {
	if ( 0+$system_id == 0) {
		$system_id = db_value('select paypal_system_id from paypal_systems where paypal_system_name=?', $system_id);	
	}


	$row = db_row('select * from paypal_systems where paypal_system_id=?', $system_id);
	$target = $row['test_mode'] ? 
			'https://www.sandbox.paypal.com/cgi-bin/webscr?cmd=_xclick' : 
			'https://www.paypal.com/cgi-bin/webscr?cmd=_xclick';

	$biz = $row['paypal_account_id'];
	$name = $extra['newtitle'];
	if (!$name) { $name = $row['paypal_system_name']; }
	$no_shipping = 1-$row['solicit_shipping_on_paypal'];
	$no_note = 1-$row['solicit_note_on_paypal'];
	$currency = $row['currency_code'];

	$tax_array = array();
	if ($extra['taxes']) {
		foreach ($extra['taxes'] as $t) {
			$tax_array[$t['sql_column']] = $t['amount'];
			$tax_total += $t['amount'];
		}
	}

	if (!$extra['return']) {
		$extra['return'] = config_get('site','home');
	}

	$pp_tid = db_newid('paypal_transactions');
	db_insert_hash('paypal_transactions', array(
		'paypal_transaction_id' => $pp_tid,
		'paypal_system_id' => $system_id,
		'user_id' => user_id(),
		'system_reference' => $item_id,
		'gross' => $price, 
		'amount_expected' => $price + $tax_total,
		'currency_expected' => $currency
	)+$tax_array);
	$pretty_item_number = $item_id;
	if (isset($extra['newitem_number'])) {
		$pretty_item_number = $extra['newitem_number'];
	}

    $custom_row = db_row('select custom_implementation,custom_implementation_header from paypal_systems where paypal_system_id=?', $system_id);
	$custom_header = $row['custom_implementation_header'];
    if ($custom_header) { require_once $custom_header; }
	$custom_function = $row['custom_implementation'];
	
	$parameters = array( 'amount'=> $price, 
			'item_name'=> $name, 
			'item_number'=> $pretty_item_number, 
			'business' => $biz,
			'no_shipping' =>$no_shipping, 
			'no_note' =>$no_note, 
			'currency_code' =>$currency, 
			'invoice' => $pp_tid, 
			'tax' => sprintf('%.02f', $tax_total), 
			'return'  => $extra['return'], 
			'lc' => strtoupper(CPF_LANGUAGE));
	if ($custom_function) {
		call_user_func($custom_function, $parameters);
	} else {
		redirect(href($target, $parameters));
	}
}

function taxes_calculate($price, $id) {
	$lang = CPF_LANGUAGE;
	if (!$lang) $lang = 'en';
	$rates = db_multirow('select tax_name_'.$lang.' as tax_name , tax_number, sql_column, tax_amount from tax_collection where tax_scheme_id=?', $id);
	$rc = array();
	foreach ($rates as $r) {
		$amt =  sprintf("%.02f", $price * $r['tax_amount']);
		$rc[] = array(
			'description' => $r['tax_name']. ($r['tax_number'] ? ' ('.$r['tax_number'].')' : ''),
			'amount' => $amt,
			'sql_column' => $r['sql_column']
		);
	}	
	return $rc;
}



function paypal_render_adminmenu() {
	# see if anything is busted
	$expected = db_value("select sum(amount_expected) from paypal_transactions as t where transaction_status='paid'");
	$received = db_value("select sum(amount_received) from paypal_notifications");

	if (0+$expected != 0+$received) {
		echo "<span class=error>Payments mismatch: expected $expected, received $received</span><br/>";
	} else { 
		printf("<p>Total sales \$%.02f</p>", $received);
	}

  $requires_attention = db_value('select count(*) from paypal_notifications where requires_action=1');
  if ($requires_attention) {
    echo '<a href="/admin/paypal/"><span class="error">Some things in PayPal need your attention.</span></a><br/>';
  }

}
?>
