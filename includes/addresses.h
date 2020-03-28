<?

function address_quality($a) {
	if ($a['street'] && $a['city'] && $a['province'] && $a['country'] && $a['postal_code']) {
			return 'G';
	} else {
			return 'U';
	}
}

function address_service_update($address_id) {
	$row = db_row('select * from user_addresses join address_services using (address_service_id) where address_id=?', $address_id);
	if (!$row) return 'no matching address';
	

	if ($row['short_name'] != 'self') {
		db_send("update user_addresses set quality='F' where address_id=?", $address_id);

		$opts = array('password'=> $row['password'],
									'user'=>$row['nickname'],
									'request'=>'getinfo');
		list ($metadata, $data) = bma_fetch($row['url'], $opts);
		if ($metadata['http_status'] != 200) {
			return "received http status ".$metadata['http_status'].' from '.$row['url'];
		} else {
			if ($data['postal']['addressNotAvailable']=='1') {
				if ( !cpf_is_email( $data['email']['address'] )) {
					return "no postal address and bad email address at remote service";
				} else {
					db_send('update user_addresses set last_update = CURRENT_TIMESTAMP, quality=?, email=? where address_id=?', 
							'J', $data['email']['address'], $address_id);
					return '';
				}
			}

			$data['postal']['province'] = $data['postal']['state'];
			$data['postal']['street'] = $data['postal']['address1'];
			$data['postal']['apartment'] = $data['postal']['address2'];
			$data['postal']['postal_code'] = $data['postal']['postalCode'];

			$quality = address_quality($data['postal']);

			db_send('update user_addresses set last_update = CURRENT_TIMESTAMP, quality=?, email=?, street=?, apartment=?, city=?, province=?, country=?, postal_code=?, delivery_instructions=?, phone=? WHERE address_id=?',
						$quality, $data['email']['address'], $data['postal']['street'], $data['postal']['apartment'], $data['postal']['city'], $data['postal']['province'], $data['postal']['country'], $data['postal']['postal_code'], 'To: '.$data['postal']['name'].'. '.$data['postal']['deliveryInstructions'], $data['postal']['phone'], $address_id);
			return '';
		}
	}

	$userid = db_value('select user_id from users where screen_name=?', $row['nickname']);
	if (!$userid) {
		db_send("update user_addresses set quality='F' where address_id=?", $address_id);
		return "no matching userid for {$row['nickname']}";
	}

	$addr = user_get_address($userid, 'main');
	$user = user_details($userid);
	db_send('update user_addresses set last_update = CURRENT_TIMESTAMP, quality=?, email=?, street=?, apartment=?, city=?, province=?, country=?, postal_code=?, delivery_instructions=?, phone=?, flowthru_address_id=? WHERE address_id=?',
				$quality, $user['email'], $addr['street'], $addr['apartment'], $addr['city'], $addr['province'], $addr['country'], $addr['postal_code'], 'To: '.user_name($userid).'. '.$addr['delivery_instructions'], $addr['phone'], $addr['address_id'], $address_id);
	return "";
}
