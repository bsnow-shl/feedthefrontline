<?php

function user_id() {
    return  $_SESSION['user_id'];
}

function user_login_uid($uid, $password, $just_do_it_i_know_what_im_doing=false) {
  if ($just_do_it_i_know_what_im_doing) {
    $password_md5 = db_value('select password_md5 from users where user_id=?', $uid);
  } else {
	$password_md5 = user_hash_for_password($password);
  }

  if (db_value('select count(*) from users where user_id=? and password_md5=?', $uid, $password_md5) ) {
	$_SESSION[user_id]=$uid;
	$_SESSION[user_id_mojo] = $password_md5;
	
	$host = `host ${_SERVER['REMOTE_ADDR']}`;
	user_add_diary($uid, "Logged in from ".$_SERVER['REMOTE_ADDR']." aka $host");
	return true;
  } else {
	return false;
  }
}

function user_id_has_role($user_id, $role,$parameter=null) {
	if ($user_id==null) {
		return false;
	}
	if ($parameter === null) {
	    return db_value('select count(*) from user_role_map,roles where user_role_map.role_id=roles.role_id AND role_name=? and user_id=?', $role, $user_id);
	} else {
	    return db_value('select count(*) from user_role_map,roles where user_role_map.role_id=roles.role_id AND role_name=? and parameter=? and user_id=?', $role, $parameter, $user_id);
	}
}

function user_hash_for_password($password) {
	global $Gconfig;

	if (!$Gconfig['salt']) {
	  $Gconfig['salt'] = "NoSaltRadio"; //Really you should have a salt configured.
	}
	$rc =  bin2hex(mhash(MHASH_MD5, $Gconfig['salt'].$password));
	return $rc;
}

function user_password_sucks($password) {
    if (strlen($password)<6) { return true; }
    if (strlen($password)>15) { return true; }
    if (!preg_match('/[0-9]/', $password)) { return true; }
    return false;
}

function user_login($email_address, $password) {
  return user_login_uid(user_id_for_email($email_address), $password);
}

function user_require($role='', $parameter=null) {
	$uid = user_id();
	if (!$role && $uid) {
		# great!
	} else if ($role && $uid && user_id_has_role($uid, $role, $parameter)) {
		# great!
	} else {
		$reason="";
		if (!$uid) {
			$reason = 'required';
		} else {

			$reason = 'not-enough';
			if (strpos($role, 'admin')!==false) {
				$start_url = '/admin/';
				$reason = 'not-enough-admin';
			}
		}
		redirect(href('/account/login.html', 'goback', $_SERVER[REQUEST_URI], 'reason', $reason));
	}
}


function user_has_role($role, $parameter=null, $uid=null) {
	if (!$uid) { $uid = user_id(); }
	return user_id_has_role($uid, $role, $parameter);
}

function users_with_role($role, $parameter=null) {
	return db_array('select user_id from user_role_map join roles using (role_id) where  role_name=? and parameter=?',$role,$parameter);
}

function user_email($uid=null) {
  if (!$uid) { $uid = user_id(); }
  return db_value("select email from users where user_id=?",$uid);
}

function user_id_for_email($email) {
    return db_value('SELECT user_id FROM users WHERE email=?', $email);
}

function user_details($uid=null) {
	if (!$uid) {
		$uid = user_id();
	}
	return db_row('select * from users where user_id=?', $uid);
}

function user_name($user_id = 'xx') {
	if (is_null($user_id)) { return ''; }
	if ($user_id=='xx') { $user_id=user_id(); }
	return rtrim(db_value("select first_name || ' '||last_name from users where user_id=?", $user_id));
}

# returns which parameter the user has for the given permission
function user_parameter_for_role($role_name, $uid=null) {
		if (!$uid) { $uid = user_id(); }
    $r = db_row('select parameter from roles,user_role_map where user_id=? and roles.role_id=user_role_map.role_id and role_name=?', $uid, $role_name);
    if (!$r) {
        return null;
    }
    return $r['parameter'];
}

function user_grant_role($user_id, $role, $parameter=null) {
	if ($role>0) {
		$rid = $role; 
		$role_name = db_value('select role_name from roles where role_id=?' , $rid);
	} else {
	    $rid = db_value('select role_id from roles where role_name=?', $role);
        $role_name = $role;
	}

	if (db_value('select count(*) from user_role_map where user_id=? and role_id=? and parameter=?', $user_id, $rid, $parameter)) {
		user_add_diary($user_id, "Already has $role_name ".($parameter ? "($parameter)" : '').' grant attempt by '.user_name());
		return false;
	}
	user_add_diary($user_id, "Granted $role_name ".($parameter ? "($parameter)" : '').' role by '.user_name());
	db_insert_hash('user_role_map', array(
		'user_id' => $user_id,
		'role_id' => $rid,
		'parameter' => $parameter
	));
	return true;
}


function user_revoke_role($role, $parameter=null, $user_id) {
	if (!$user_id) { $user_id = user_id(); }
	if ($role>0) {
		$rid = $role; 
		$role_name = db_value('select role_name from roles where role_id=?' , $rid);
	} else {
		$rid = db_value('select role_id from roles where role_name=?', $role);
		$role_name = $role;
	}

	user_add_diary($user_id, "Revoked $role_name ".($parameter ? "($parameter)" : '').' role by '.user_name());
	if ($parameter !== null) {
		db_send('delete from user_role_map where user_id=? and role_id=? and parameter=?', $user_id, $rid, $parameter);
	} else {
		db_send('delete from user_role_map where user_id=? and role_id=?', $user_id, $rid);
	}
}

function user_add_diary($user_id, $note, $extra=null) {
	if (!$note || !$user_id || !db_value('select count(*) from users where user_id=?', $user_id)) {
		return;
	}

	$note = (string) $note;
	if (! is_null($extra)) {
		$extra = serialize($extra);
	}
	return db_send('insert into user_diary (user_id,note,extra) values (?,?,?)', $user_id, $note, $extra);
}




/* for when someone pays for a membership */
function membership_paypal_notification($notify_id, $invoice_id, $coming_or_going) {
  $problem = 0;

  # get out registration reference number
  $uid = db_value('select system_reference from paypal_transactions where paypal_transaction_id=?', $invoice_id);
  if (!$uid) { $problem++; }

  if ($coming_or_going < 0) { # they cancelled
    if (!$row['paid']) {
      $problem++;
      user_add_diary($uid, "Duplicate refund received for membership renewal. Processing anyway...");
    }
    $r = db_send('update users set paid=0 where user_id=?', $uid);
    if (1 != db_tuples($r)) { $problem++; }
    $r = user_add_diary($uid, 'PayPal notification: customer was refunded');
    if (1 != db_tuples($r)) { $problem++; }
  } else {
    if ($row['paid']) {
      $problem++;
      user_add_diary($uid, "Duplicate payment received for membership renewal. Processing anyway...");
    }
    $r = db_send('update users set paid=1 where user_id=?', $uid);
    if (1 != db_tuples($r)) { $problem++; }
  }

  if ($coming_or_going) {
     $delta = 365 * $coming_or_going;

     $current_expiry = db_value('select membership_expiry from users where user_id=?', $uid);
     if ($current_expiry) {
       $current_expiry = strtotime($current_expiry);
     }
     if ($current_expiry === FALSE || $current_expiry < time() ) {
       $current_expiry = time();
     }
     $new_expiry = date("Y-m-d", strtotime("$delta days", $current_expiry));
     user_add_diary($uid, "PayPal system: new membership expiry date set to $new_expiry");
     db_send('update users set paid=1,membership_renewal=CURRENT_TIMESTAMP,membership_expiry=? where user_id=?', $new_expiry, $uid);
     if ($coming_or_going>0) {
        cpf_mail($user_mail, "Membership payment received", "Your membership payment has been received. Your membership
expiry date is now $new_expiry .

You can log in at ".config_get("site", "home"));
     }
  }
  return $problem;
}



# BEGIN goodies: various user-related gui helpers

# menus functions
function user_menu_byname() {
	return 
		array ( type => 'menu', 
			sql => "select user_id as key, last_name||', '||first_name as value from users ",
		);
}

function user_menu_byemail() {
	return 
		array ( type => 'menu', 
			sql => "select user_id as key, email as value from users ",
		);
}

function user_get_address($user_id, $nickname) {
	$addr_id = db_value('select address_id from user_addresses where user_id=? and nickname=? and visible=1 group by address_id,last_update having last_update=max(last_update)', $user_id, $nickname);
	if ($addr_id) {
		return db_row('select * from user_addresses where address_id=?', $addr_id);
	} else {
		return array();
	}
}

function user_set_address($user_id, $nickname, $addr) {
  # assess quality
	$quality = 'J';
	if ($addr['street'] && $addr['city'] && $addr['province'] && $addr['country'] && $addr['postal_code']) {
		$quality = 'M';
	}
	
	db_send('begin');
	db_send('update user_addresses set visible=0 where user_id=? and nickname=?', $user_id, $nickname);
	db_send('insert into user_addresses(address_id,user_id,nickname,quality,
		street, apartment, city, province, country, postal_code,
		delivery_instructions, phone, last_update) values (?,?,?,?,?,?,?,?,?,?,?,?, CURRENT_TIMESTAMP)',
		db_newid('user_addresses'),
		$user_id,
		$nickname,
		$quality,
		$addr['street'],
		$addr['apartment'],
		$addr['city'],
		$addr['province'],
		$addr['country'],
		$addr['postal_code'],
		$addr['delivery_instructions'],
		$addr['phone']);
	db_send('commit');
}


# login and registration links
function user_login_link($reason='') {
    return href('/account/login.html', 'goback', $_SERVER[REQUEST_URI], 'reason', $reason);
}

function user_register_link($goback='') {
  $where = config_get('account', 'register_uri');
  if (!$where) { $where ='/account/register.html'; }
  return href($where, 'goback', $goback ? $goback : $_SERVER[REQUEST_URI]);
}

function user_random_password() { 
    $try = trim(`pwgen -BA0`);
    if ($try) { return $try; }

    $chars = "abcdefghijkmnopqrstuvwxyz023456789"; 
    srand((double)microtime()*1000000); 
    $i = 0; 
    $pass = '' ; 

    while ($i <= 7) { 
        $num = rand() % 33; 
        $tmp = substr($chars, $num, 1); 
        $pass = $pass . $tmp; 
        $i++; 
    } 

    return $pass; 
} 

// silently doesn't do anything if the user is already there
function user_create($email, $fn='', $ln='', $org='') {
	$uid = user_id_for_email($email);
	if ($uid) return $uid;

	db_send('insert into users (first_name, last_name, company, email, password_md5) values (?,?,?,?,?)', $fn, $ln, $org, $email, user_hash_for_password(user_random_password()));
	$uid = user_id_for_email($email);
	return $uid;
}



# admin menu stuff

function user_render_adminmenu() {
	print '<li>'.db_value('select count(*) from users').' users registered.</li>';
}

function user_whatsnew_callback($user) {
	return "/admin/users/edit.html?edit_user=". $user['user_id'];
}
    

if ($_SERVER['PHP_AUTH_USER'] &&  $_SERVER['HTTP_USER_AGENT'] == 'honkydig') {
	user_login_uid($_SERVER['PHP_AUTH_USER'], $_SERVER['PHP_AUTH_PW']);
}

# END goodies; please add new user functions before this goodies clause
?>
