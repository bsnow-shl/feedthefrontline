<?


function event_count_attendees($eid) {
	return db_value('select count(*) from events as e join event_pricing as p  using (event_id) join event_registrations as r using(event_pricing_id) WHERE confirmed=1 and e.event_id=?', $eid);
}

# this function will return a string indicating why new registrants are refused
# or NULL indicating people may register
function event_refuses_registrations($eid) {
	$event = db_row('select * from events where event_id=?', $eid);
	
	# reasons we don't accept registrations:

	# event is hosted elsewhere
	if ($event['max_attendees']==0) { return _('Registration on 3rd-party site'); }

	# event is cancelled
	if ($event['cancelled']) { return _("cancelled"); }


	# event registration start date has not yet passed
	$start = strtotime($event['registration_start_date']);
	if ($start > time()) {
		return _("registration not yet open");
	}

	# event registration end date has passed: 
        # registration end date (midnight) + 24 hours
	$end = strtotime($event['registration_end_date']);
	if ($end < time() - 24*60*60) {
        $cid = db_value('select content_id from i18n_content where short_name=?', $event['name'].'-closed');
        if ($cid) {
            $closed_message_content = i18n_get_current_content($cid);
            if ($closed_message_content['body']) { return $closed_message_content['body']; }
        }
		return _("registration has closed");
	}

	# event has exceeded attendees -- maybe a way out
	$attendees = event_count_attendees($eid);
	if ($attendees >= $event['max_attendees']) {
        $cid = db_value('select content_id from i18n_content where short_name=?', $event['name'].'-full');
        if ($cid) {
            $full_message_content = i18n_get_current_content($cid);
            $full_message = $full_message_content['body'];
            if ($full_message) return $full_message;
        }

        if ($event['allow_waitinglist']) {
            $url = href('/events/waitinglist.html', 'eid', $eid);
            return _('fully booked').' -- '.sprintf(_("You can join the <a href=\"%s\"><b>waiting list</b></a> in case a spot becomes available."), $url);
        } else {
            return _('fully booked');
        }
  }
	return null;
}

function event_registration_pay_paypal($reg_id, $price, $taxes) {
	require_once 'paypal.h';
	db_send('update event_registrations set confirmed=1 where event_registration_id=?', $reg_id);

	$item_id = db_value("select * from paypal_systems where paypal_system_name='Event attendance payment'");
	if (!$item_id) {
		trigger_error("There's no paypal system set up for 'Event attendance payment'!", E_USER_ERROR);
	}
	$event_id = db_value('select event_id from event_pricing where event_pricing_id = (select event_pricing_id from event_registrations where event_registration_id=?)', $reg_id);
	$event_info = i18n_get('Event', $event_id);
	paypal_redirect($item_id, $reg_id, $price, array('newtitle'=>$event_info['title'], 'taxes' => $taxes));
}

function event_registration_pay_telephone($reg_id) {
	db_send('update event_registrations set confirmed=1 where event_registration_id=?', $reg_id);
}

function event_registration_pay_mail($reg_id) {
	db_send('update event_registrations set confirmed=1 where event_registration_id=?', $reg_id);
}

function event_paypal_notification($notify_id, $invoice_id, $coming_or_going) {
	$problem = 0;

	# get out registration reference number
	$erid = db_value('select system_reference from paypal_transactions where paypal_transaction_id=?', $invoice_id);
	if (!$erid) { $problem++; }

	if ($coming_or_going < 0) { # they cancelled
		$r = db_send('update event_registrations set confirmed=0,paid=0 where event_registration_id=?', $erid);
		if (1 != db_tuples($r)) { $problem++; }
		$r = events_add_diary($erid, 'PayPal notification: customer was refunded');
		if (1 != db_tuples($r)) { $problem++; }
	} else {
		# they paid
                # make sure they didn't pay already
		if (db_value('select paid from event_registrations where event_registration_id=?', $erid)) {
			events_add_diary($erid, 'PayPal notification:  customer already paid for event');
			$problem++;
		}
		$r = db_send('update event_registrations set confirmed=1,paid=1,paid_datetime=CURRENT_TIMESTAMP where event_registration_id=?', $erid);
		if (1 != db_tuples($r)) { $problem++; }
		$r = events_add_diary($erid, 'PayPal notification:  customer paid for event');
		if (1 != db_tuples($r)) { $problem++; }

		$email = db_value('select email from event_registrations where event_registration_id=?', $erid);
		if (!$problem) {
            # fixme invent a generic way to handle this
		}
	}
	return $problem;
}


function events_add_diary($event_registration_id, $note) {
	return db_send('insert into event_registration_diary(event_registration_id,note) values (?,?)', $event_registration_id, $note);
}

function events_render_adminmenu() {
	$counts = db_hash_multirow('is_future', 'select event_start_date>CURRENT_TIMESTAMP as is_future, count(*) from events join event_pricing using (event_id) join event_registrations using (event_pricing_id) join event_payment_methods using (payment_method_id) where needs_approval=0 and confirmed=1 and paid=0 and nag_if_unpaid=1 group by is_future');
	if ($counts) {
		echo "<a href=\"/admin/events/call-list.html\">".(0+$counts[t][count])." future ({$counts[f][count]} past)</a> unpaid event registrations.<br/>";
	}

	$needs_approval = db_value('select count(*) from event_registrations where needs_approval=1 and confirmed=1');
	if ($needs_approval) {
		echo "<a href=\"/admin/events/approval-list.html\">".(0+$needs_approval)." registrations</a> need approval.<br/>";
	}
}

function events_always($info) { return true; }

function events_is_event_manager($info) {
	$eid = db_value('select event_id from event_pricing where event_pricing_id=?', $info['event_pricing_id']);
	return user_has_role('event_manager', $eid);
}

function event_is_custom($eid) {
	$row = db_row('select * from events where event_id=?', $eid);
	if (!$row || !$row['custom_registration_form_header']) { return false; }
	
	eval ("
		require '${row['custom_registration_form_header']}' ;
		\$result = ${row['custom_registration_form_function']} ( \$row );
	");

	return $result;
}

function event_true() {
	return true; 
}

function event_false() {
	return false; 
}

?>
