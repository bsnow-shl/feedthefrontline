<?php
require_once 'paypal.h';

// read the post from PayPal system and add 'cmd'
$req = 'cmd=_notify-validate';

foreach ($_POST as $key => $value) {
	$value = urlencode(stripslashes($value));
	$req .= "&$key=$value";
}

// post back to PayPal system to validate
$header .= "POST /cgi-bin/webscr HTTP/1.0\r\n";
$header .= "Content-Type: application/x-www-form-urlencoded\r\n";
$header .= "Content-Length: " . strlen($req) . "\r\n\r\n";

$invoice = $_POST['invoice'];

# determine if the invoice is from test or not
$test_mode = db_value('select test_mode from paypal_transactions join paypal_systems using (paypal_system_id) 
	where paypal_transaction_id=?', $invoice);
$server = $test_mode ? 'www.sandbox.paypal.com': 'www.paypal.com';


$fp = fsockopen ($server, 80, $errno, $errstr, 30);

// assign posted variables to local variables
//$item_name = $_POST['item_name'];  // ignored
//$item_number = $_POST['item_number'];  // ignored
$payment_status = $_POST['payment_status'];
$payment_amount = $_POST['mc_gross'];
$payment_currency = $_POST['mc_currency'];
$txn_id = $_POST['txn_id'];
$parent_txn_id = $_POST['parent_txn_id'];

# some things to be established by the next code block
$is_authentic = 0;
$requires_action = 1;
$deliver_notification = 0;
$credit_or_debit = 0;   // 1 == log funds in or out (paypal will mark them negative for refunds/disputes);  0=informational
$new_status = '';

# log it
if (!$fp) {
	paypal_add_diary_note($invoice, 
		"Outgoing HTTP error while verify PayPal IPN: $fp");
	$requires_action = 1;
} else {
	fputs ($fp, $header . $req);
	while (!feof($fp)) {
		$res = fgets ($fp, 1024);
		if (strcmp ($res, "VERIFIED") == 0) {
			$is_authentic=1;
			$requires_action = 0;

			paypal_add_diary_note($invoice, 
				"Paypal said payment_status is $payment_status.");
	
			if ($payment_status == 'Completed' || $payment_status=='Canceled_Reversal') {
				$credit_or_debit = 1;
				$new_status='paid';

				# if the old status was "paid", something's wrong
				$trans_row = db_row('select currency_expected, amount_expected,transaction_status from paypal_transactions where paypal_transaction_id=?', $invoice);
				if ($trans_row['status'] == 'paid') {
								$requires_action=1;
								paypal_add_diary_note($invoice,
									"Received duplicate payment notification.");
				}

				# if the currency doesn't match, something's wrong
				$expected_currency = $trans_row['currency_expected'];
				if ($expected_currency != $payment_currency) {
					$requires_action=1;
					paypal_add_diary_note($invoice,
						"Currency received ($payment_currency) is not the expected currency of $expected_currency.");
				} else {
					$deliver_notification = 1;
				}

				# if the amount doesn't match, something's wrong
				$expected_amount = $trans_row['amount_expected'];
				if (!config_get('paypal','different_amounts_ok') && $expected_amount != $payment_amount) {
					$requires_action=1;
					paypal_add_diary_note($invoice,
						"Amount received ($payment_amount) is not the expected amount.");
				} else {
					$deliver_notification = 1;
				}
			} else if ($payment_status == 'Reversed') {
				$credit_or_debit = 1;
				$requires_action = 1;
				$deliver_notification = -1;
				paypal_add_diary_note($invoice, 
					"Reversed reason is ".$_POST['reason_code']);
				$new_status = 'disputed';
			} else if ($payment_status == 'Refunded') {
				$credit_or_debit = 1;
				$requires_action = 0;
				$deliver_notification = -1;
				paypal_add_diary_note($invoice, 
					"Refunded reason is ".$_POST['reason_code']);
				$new_status = 'refunded';

				# if the previous status wasn't paid, this needs notification
				$trans_row = db_row('select amount_expected,transaction_status from paypal_transactions where paypal_transaction_id=?', $invoice);
				if ($trans_row['transaction_status'] != 'paid') {
					$requires_action=1;
					paypal_add_diary_note($invoice,
						"Received refund notice without the transaction being marked paid.");
				}
			} else if ($payment_status == 'Pending') {
				$requires_action=0;
				paypal_add_diary_note($invoice, 
					'Pending reason is '.$_POST['pending_reason']);
			} else {
				$requires_action=1;
				paypal_add_diary_note($invoice, 
					'Unrecognized payment status of '.$payment_status);
			}
		}
		else if (strcmp ($res, "INVALID") == 0) {
			// log for manual investigation
			$is_authentic = 0;
			$requires_action = 1;
			paypal_add_diary_note($invoice, 
				"PayPal denied the authenticity of the message we received, possibly an error on their end.");
		}
	}
	fclose ($fp);
}


$notify_id = db_newid('paypal_notifications');
db_insert_hash('paypal_notifications', array(
	'paypal_notification_id' => $notify_id,
	'paypal_transaction_id' => $invoice,
	'paypal_txnid' => $txn_id,
	'paypal_parent_txnid' => $parent_txn_id,
	'requires_action' => $requires_action,
	'is_authentic' => $is_authentic,
	'amount_received' => $payment_amount * $credit_or_debit,
));
foreach ($_POST as $k=>$v) {
	db_send('insert into paypal_notification_data (paypal_notification_id,key,value) values (?,?,?)', $notify_id, $k, $v);
}
if ($new_status) {
	db_send('update paypal_transactions set transaction_status=? where paypal_transaction_id=?', $new_status, $invoice);
}


function paypal_error_handler($errno, $errstr, $errfile, $errline) {
  global $paypal_callback_errors;

  if ($errno == E_NOTICE && preg_match('/^(Constant|Undefined variable|Undefined index|Use of undefined constant)/', $errstr)) return;
  $paypal_callback_errors[] = array('message' => $errstr, 'file' => $errfile, 'line' => $errline);
}


if ($deliver_notification) {
	$system_info = db_row('select notification_header,notification_function from paypal_systems where paypal_system_id=(select paypal_system_id from paypal_transactions where paypal_transaction_id=?)', $invoice);
	$header = $system_info['notification_header'];
	$function = $system_info['notification_function'];

  set_error_handler("paypal_error_handler");

	if ($header) { require_once "$header"; }
	if ($function) { $result =  call_user_func($function, $notify_id, $invoice, $deliver_notification); }
	if ($result || $paypal_callback_errors) {
		# has to return 0 if everything is okay
		db_send('update paypal_notifications set requires_action=1 where paypal_notification_id=?', $notify_id);
		paypal_add_diary_note($invoice, "Calling dependent system failed: ($function, $notify_id, $invoice, $deliver_notification) yielded $result");
    foreach ($paypal_callback_errors as $e => $err) {
      paypal_add_diary_note($invoice, sprintf("dependent error: %s %s (line %d)", $err['message'], $err['file'], $err['line']));
    }
	}
  restore_error_handler();
}


?>
