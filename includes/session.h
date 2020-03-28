<?php
require_once 'db.h';

function _cpfsess_open($path, $name) {
	return true;
}

function _cpfsess_close() {
	return true;
}

function _cpfsess_read ($key) {
  $result = db_value("SELECT session_data 
		FROM sessions WHERE session_id=? and expiry>CURRENT_TIMESTAMP",$key);
  if ($result) {
	  return $result;
  } else {
	  return '';
  }
}

function _cpfsess_write ($key, $value) {
  db_send('begin');
  $result = db_send("UPDATE sessions SET session_data=? WHERE session_id=? and expiry>CURRENT_TIMESTAMP",$value,$key);
  if (!db_tuples($result)) {
    # doh.  The session wasn't there.  Insert it.
    _cpfsess_gc(0);
    db_send("INSERT INTO sessions(session_id, session_data) VALUES (?,?)",$key,$value);
  }
  db_send('commit');
}

function _cpfsess_destroy ($key) {
  db_send("DELETE FROM sessions WHERE session_id=?",$key);
}

function _cpfsess_gc ($maxlifetime) {
  db_send("DELETE FROM sessions WHERE expiry < 'now'::timestamp");
}

session_set_save_handler('_cpfsess_open', 
		'_cpfsess_close', 
		'_cpfsess_read', 
		'_cpfsess_write', 
		'_cpfsess_destroy', 
		'_cpfsess_gc');

if (substr($_SERVER['REQUEST_URI'],0,5)!='/blog') {
	session_set_cookie_params($Gconfig['session_lifetime'], '/', $Gconfig['session_domain']);
	session_start();
}

if ($_SESSION['user_id']) {
	require_once 'users.h';

	if ($_SESSION['user_id_mojo'] != db_value('select password_md5 from users where user_id=?', $_SESSION[user_id])) {
		unset ($_SESSION[user_id]);
	}
}

?>
